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
#include <miopen/kernel_build_params.hpp>
#include <miopen/smoothl1loss/invoke_params.hpp>
#include <miopen/smoothl1loss/solvers.hpp>
#include <miopen/smooth_l1loss.hpp>
#include <miopen/target_properties.hpp>
#include <miopen/tensor_view_5d.hpp>

#define LOCAL_SIZE_NONCONTIGUOUS_BWD 256
#define LOCAL_SIZE_CONTIGUOUS_BWD 256

namespace miopen {

namespace solver {

const auto make_hip_kernel = [](std::vector<size_t> localsize,
                                std::vector<size_t> gridsize,
                                std::string kernel_file,
                                std::string kernel_name,
                                KernelBuildParameters build_params) {
    while(localsize.size() < 3)
        localsize.push_back(1);
    while(gridsize.size() < 3)
        gridsize.push_back(1);
    for(int i = 0; i < localsize.size(); ++i)
        gridsize[i] = AlignUp(gridsize[i], localsize[i]);
    return KernelInfo{
        build_params.GenerateFor(kbp::HIP{}), localsize, gridsize, kernel_file, kernel_name};
};

namespace smoothl1loss {

bool SmoothL1LossUnreducedBackwardSolver::IsApplicable(
    const ExecutionContext& /*context*/,
    const miopen::smoothl1loss::UnreducedBackwardProblemDescription& problem) const
{
    if(!problem.IsSameType())
        return false;
    if(!problem.IsRightLength())
        return false;
    if(!problem.IsRightStride())
        return false;
    return true;
}

bool SmoothL1LossUnreducedBackwardContiguous::IsApplicable(
    const ExecutionContext& context,
    const miopen::smoothl1loss::UnreducedBackwardProblemDescription& problem) const
{
    if(!problem.IsSameStride() && !problem.IsAllContiguous())
        return false;
    if(!SmoothL1LossUnreducedBackwardSolver::IsApplicable(context, problem))
        return false;
    return true;
}

ConvSolution SmoothL1LossUnreducedBackwardContiguous::GetSolution(
    const ExecutionContext& /*context*/,
    const miopen::smoothl1loss::UnreducedBackwardProblemDescription& problem) const
{
    auto result = ConvSolution{miopenStatusSuccess};

    auto dtype        = problem.GetDIDesc().GetType();
    auto input_dtype  = miopen::GetDataType(problem.GetIDesc().GetType());
    auto output_dtype = miopen::GetDataType(problem.GetDODesc().GetType());
    auto size         = problem.GetDIDesc().GetElementSize();

    result.construction_params.push_back(
        make_hip_kernel({LOCAL_SIZE_CONTIGUOUS_BWD},
                        {size},
                        "MIOpenSmoothL1Loss.cpp",
                        "SmoothL1LossUnreducedBackwardContiguous",
                        KernelBuildParameters{
                            {"MIOPEN_USE_FP16", static_cast<int>(dtype == miopenHalf)},
                            {"MIOPEN_USE_FP32", static_cast<int>(dtype == miopenFloat)},
                            {"MIOPEN_USE_FP64", static_cast<int>(dtype == miopenDouble)},
                            {"MIOPEN_USE_BFP16", static_cast<int>(dtype == miopenBFloat16)},
                            {"INPUT_TYPE", input_dtype == "bfloat16" ? "ushort" : input_dtype},
                            {"OUTPUT_TYPE", output_dtype == "bfloat16" ? "ushort" : output_dtype},
                        }));

    result.invoker_factory = [](const std::vector<Kernel>& kernels) {
        return [=](const Handle& handle_, const AnyInvokeParams& raw_params) {
            decltype(auto) kernel = handle_.Run(kernels.front());
            decltype(auto) params = raw_params.CastTo<miopen::smoothl1loss::InvokeParams>();

            auto size = params.iDesc->GetElementSize();

            kernel(
                params.i, params.t, params.o_grad, params.i_grad, params.t_grad, params.beta, size);
        };
    };

    return result;
}

bool SmoothL1LossReducedBackward5d::IsApplicable(
    const ExecutionContext& /*context*/,
    const miopen::smoothl1loss::ReducedBackwardProblemDescription& problem) const
{
    if(problem.GetIDesc().GetSize() > 5)
        return false;
    if(!problem.IsSameType())
        return false;
    if(!problem.IsRightLength())
        return false;
    if(!problem.IsRightStride())
        return false;
    return true;
}

ConvSolution SmoothL1LossReducedBackward5d::GetSolution(
    const ExecutionContext& /*context*/,
    const miopen::smoothl1loss::ReducedBackwardProblemDescription& problem) const
{
    auto result = ConvSolution{miopenStatusSuccess};

    auto dtype        = problem.GetDIDesc().GetType();
    auto input_dtype  = miopen::GetDataType(problem.GetIDesc().GetType());
    auto output_dtype = miopen::GetDataType(problem.GetDODesc().GetType());
    auto size         = problem.GetIDesc().GetElementSize();

    auto build_params = KernelBuildParameters{
        {"MIOPEN_USE_FP16", static_cast<int>(dtype == miopenHalf)},
        {"MIOPEN_USE_FP32", static_cast<int>(dtype == miopenFloat)},
        {"MIOPEN_USE_FP64", static_cast<int>(dtype == miopenDouble)},
        {"MIOPEN_USE_BFP16", static_cast<int>(dtype == miopenBFloat16)},
        {"INPUT_TYPE", input_dtype == "bfloat16" ? "ushort" : input_dtype},
        {"OUTPUT_TYPE", output_dtype == "bfloat16" ? "ushort" : output_dtype}};

    result.construction_params.push_back(make_hip_kernel({LOCAL_SIZE_NONCONTIGUOUS_BWD},
                                                         {size},
                                                         "MIOpenSmoothL1Loss.cpp",
                                                         "SmoothL1LossReducedBackward5d",
                                                         build_params));

    result.invoker_factory = [](const std::vector<Kernel>& kernels) {
        return [=](const Handle& handle_, const AnyInvokeParams& raw_params) {
            decltype(auto) kernel = handle_.Run(kernels.front());
            decltype(auto) params = raw_params.CastTo<miopen::smoothl1loss::InvokeParams>();

            auto I_tv  = get_inner_expanded_tv(deref(params.iDesc));
            auto T_tv  = get_inner_expanded_tv(deref(params.tDesc));
            auto dI_tv = get_inner_expanded_tv(deref(params.iDesc));
            auto dT_tv = get_inner_expanded_tv(deref(params.tDesc));

            handle_.ResetKernelTime();
            kernel(params.i,
                   params.t,
                   params.o_grad,
                   params.i_grad,
                   params.t_grad,
                   params.beta,
                   params.divisor,
                   I_tv,
                   T_tv,
                   dI_tv,
                   dT_tv);
        };
    };

    return result;
}

} // namespace smoothl1loss

} // namespace solver

} // namespace miopen
