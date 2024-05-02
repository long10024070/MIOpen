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

#include <miopen/datatype.hpp>
#include <miopen/find_solution.hpp>
#include <miopen/float_equal.hpp>
#include <miopen/kernel_cache.hpp>
#include <miopen/smoothl1loss/invoke_params.hpp>
#include <miopen/smoothl1loss/solvers.hpp>
#include <miopen/smooth_l1loss.hpp>
#include <miopen/tensor.hpp>

namespace miopen {

size_t GetSmoothL1LossReducedWorkspaceSize(Handle& handle,
                                           const TensorDescriptor& iDesc,
                                           const TensorDescriptor& tDesc,
                                           const TensorDescriptor& oDesc)
{
    auto ctx           = ExecutionContext{&handle};
    const auto problem = smoothl1loss::ReducedProblemDescription{iDesc, tDesc, oDesc};

    const auto algo = AlgorithmName{"SmoothL1LossReducedForward"};
    const auto solvers =
        solver::SolverContainer<solver::smoothl1loss::SmoothL1LossReducedForward5d>{};

    auto pair_size_vector = solvers.GetWorkspaceSizes(ctx, problem);

    return pair_size_vector.empty() ? static_cast<size_t>(-1) : pair_size_vector.front().second;
}

miopenStatus_t SmoothL1LossReducedForward(Handle& handle,
                                          Data_t workspace,
                                          size_t workspaceSizeInBytes,
                                          const TensorDescriptor& iDesc,
                                          ConstData_t i,
                                          const TensorDescriptor& tDesc,
                                          ConstData_t t,
                                          const TensorDescriptor& oDesc,
                                          Data_t o,
                                          float beta,
                                          float divisor)
{
    const auto problem = smoothl1loss::ReducedProblemDescription{iDesc, tDesc, oDesc};

    const auto invoke_params = [&]() {
        auto tmp           = smoothl1loss::InvokeParams{};
        tmp.type           = InvokeType::Run;
        tmp.iDesc          = &iDesc;
        tmp.tDesc          = &tDesc;
        tmp.oDesc          = &oDesc;
        tmp.i              = i;
        tmp.t              = t;
        tmp.o              = o;
        tmp.workspace      = workspace;
        tmp.workspace_size = workspaceSizeInBytes;
        tmp.beta           = beta;
        tmp.divisor        = divisor;
        return tmp;
    }();

    const auto algo = AlgorithmName{"SmoothL1LossReducedForward"};
    const auto solvers =
        solver::SolverContainer<solver::smoothl1loss::SmoothL1LossReducedForward5d>{};

    solvers.ExecutePrimitive(handle, problem, algo, invoke_params);

    return miopenStatusSuccess;
}

miopenStatus_t SmoothL1LossUnreducedForward(Handle& handle,
                                            const TensorDescriptor& iDesc,
                                            ConstData_t i,
                                            const TensorDescriptor& tDesc,
                                            ConstData_t t,
                                            const TensorDescriptor& oDesc,
                                            Data_t o,
                                            float beta)
{
    const auto problem = smoothl1loss::UnreducedProblemDescription{iDesc, tDesc, oDesc, beta};

    const auto invoke_params = [&]() {
        auto tmp  = smoothl1loss::InvokeParams{};
        tmp.type  = InvokeType::Run;
        tmp.iDesc = &iDesc;
        tmp.tDesc = &tDesc;
        tmp.oDesc = &oDesc;
        tmp.i     = i;
        tmp.t     = t;
        tmp.o     = o;
        tmp.beta  = beta;
        return tmp;
    }();

    const auto algo = AlgorithmName{"SmoothL1LossUnreducedForward"};
    const auto solvers =
        solver::SolverContainer<solver::smoothl1loss::SmoothL1LossUnreducedForwardContiguous,
                                solver::smoothl1loss::SmoothL1LossUnreducedForward5d>{};

    solvers.ExecutePrimitive(handle, problem, algo, invoke_params);

    return miopenStatusSuccess;
}

} // namespace miopen