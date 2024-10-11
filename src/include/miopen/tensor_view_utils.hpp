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

#ifndef MIOPEN_TENSOR_VIEW_UTIL_HPP_
#define MIOPEN_TENSOR_VIEW_UTIL_HPP_

#include <miopen/common.hpp>
#include <miopen/tensor.hpp>
#include "../../kernels/tensor_view.hpp"
#include "miopen/errors.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

namespace miopen {

template <int N>
inline tensor_view_t<N> get_inner_expanded_tv(const TensorDescriptor Desc)
{
    auto dims    = Desc.GetLengths();
    auto strides = Desc.GetStrides();

    tensor_view_t<N> tensor_view;
    for(size_t i = 0; i < N; ++i)
    {
        if(i < dims.size())
        {
            tensor_view.stride[i] = strides[i];
            tensor_view.size[i]   = dims[i];
        }
        else
        {
            tensor_view.stride[i] = (i == 0 ? 1 : strides[i - 1]);
            tensor_view.size[i]   = 1;
        }
    }
    return tensor_view;
}

template <int N>
inline void slice_tv(tensor_view_t<N>& tensor_view, int32_t sliceCount, const int32_t* slices)
{
    for(int32_t i = 0; i < sliceCount; i++)
    {
        int32_t dim   = slices[4 * i + 0];
        int32_t start = slices[4 * i + 1];
        int32_t end   = slices[4 * i + 2];
        int32_t step  = slices[4 * i + 3];

        if(end > static_cast<int32_t>(tensor_view.size[dim]))
            end = tensor_view.size[dim];

        auto len = end - start;

        tensor_view.size[dim] = (len + step - 1) / step;
        tensor_view.stride[dim] *= step;
    }
}

template <int N, typename T>
inline void permute_tv(tensor_view_t<N>& tensor_view, std::vector<T> permute)
{
    // Validate permutation
    {
        MIOPEN_THROW_IF(
            permute.size() != N,
            (std::stringstream() << "Tensor view permute: Permutation size must be " << N).str());
        std::vector<bool> exist(N, false);
        for(auto idx : permute)
        {
            MIOPEN_THROW_IF(idx < 0 || N <= idx,
                            (std::stringstream()
                             << "Tensor view permute: Permutation value must be in range [" << 0
                             << "," << N - 1 << "], while it is " << idx)
                                .str());
            MIOPEN_THROW_IF(exist[idx],
                            (std::stringstream()
                             << "Tensor view permute: Permutation value " << idx << " duplicate.")
                                .str());
            exist[idx] = true;
        }
    }

    uint64_t new_stride[N], new_size[N];
    for(auto i = 0; i < N; ++i)
    {
        new_stride[i] = tensor_view.stride[permute[i]];
        new_size[i]   = tensor_view.size[permute[i]];
    }
    std::copy(new_stride, new_stride + N, tensor_view.stride);
    std::copy(new_size, new_size + N, tensor_view.size);
}

} // namespace miopen

#endif // MIOPEN_TENSOR_REORDER_UTIL_HPP_
