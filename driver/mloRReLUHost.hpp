/*******************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2024 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/

#pragma once

#include <../test/ford.hpp>

#include <miopen/tensor.hpp>
#include <miopen/rrelu/utils.hpp>

#include "dropout_gpu_emulator.hpp"

template <typename Tgpu, typename Tcheck>
int32_t mloRReLUForward5dRunHost(const std::vector<prngStates>& states,
                                 const miopenTensorDescriptor_t inputDesc,
                                 const miopenTensorDescriptor_t outputDesc,
                                 const Tgpu* input,
                                 Tcheck* output_host,
                                 float* noise_host,
                                 const float lower,
                                 const float upper)
{
    auto input_tv  = miopen::solver::rrelu::get_inner_expanded_tv<5>(miopen::deref(inputDesc));
    auto output_tv = miopen::solver::rrelu::get_inner_expanded_tv<5>(miopen::deref(outputDesc));

    int num_states = states.size();

    int num_threads;
    {
        size_t size = miopen::deref(inputDesc).GetElementSize();
        if(size <= num_states)
            num_threads = size;
        else
        {
            size_t divisor = 1;
            while((1ul << divisor) * divisor <= size)
                ++divisor;
            --divisor;
            num_threads = (1ul << divisor);
        }
    }
    num_threads = AlignUp(num_threads, 256);

    par_ford(num_threads)([&](int gid) {
        prngStates curState = states[gid % num_states];
        for(int i = gid; i < miopen::deref(inputDesc).GetElementSize(); i += num_threads)
        {
            auto layout = tensor_layout_t<5>(input_tv, i);
            auto Iidx   = input_tv.get_tensor_view_idx(layout);
            auto Oidx   = output_tv.get_tensor_view_idx(layout);
            auto x      = static_cast<float>(input[Iidx]);

            float alpha = 1.0f;
            if(x < 0.0f)
                // This part is copied from Dropout operation
                alpha = uniform_distribution_emu(xorwow_next(&curState)) * (upper - lower) + lower;

            output_host[Oidx] = static_cast<Tcheck>(x * alpha);
            noise_host[i]     = alpha;
        }
    });

    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tcheck>
int32_t mloRReLUBackward5dRunHost(const miopenTensorDescriptor_t doutputDesc,
                                  const miopenTensorDescriptor_t dinputDesc,
                                  const float* noise,
                                  const Tgpu* doutput,
                                  Tcheck* dinput_host)
{
    auto doutput_tv = miopen::solver::rrelu::get_inner_expanded_tv<5>(miopen::deref(doutputDesc));
    auto dinput_tv  = miopen::solver::rrelu::get_inner_expanded_tv<5>(miopen::deref(dinputDesc));

    int size = miopen::deref(doutputDesc).GetElementSize();
    par_ford(size)([&](int i) {
        auto layout = tensor_layout_t<5>(dinput_tv, i);
        auto dIidx  = dinput_tv.get_tensor_view_idx(layout);
        auto dOidx  = doutput_tv.get_tensor_view_idx(layout);

        dinput_host[dIidx] = static_cast<Tcheck>(static_cast<float>(doutput[dOidx]) / noise[i]);
    });

    return miopenStatusSuccess;
}
