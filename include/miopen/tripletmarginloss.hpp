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

#include <miopen/common.hpp>

namespace miopen {

struct Handle;
struct TensorDescriptor;

size_t GetTripletMarginLossForwardWorkspaceSize(Handle& handle,
                                                const TensorDescriptor& aDesc,
                                                const TensorDescriptor& oDesc);

miopenStatus_t TripletMarginLossForward(Handle& handle,
                                        Data_t workspace,
                                        size_t workspaceSizeInBytes,
                                        const TensorDescriptor& aDesc,
                                        ConstData_t anchor,
                                        const TensorDescriptor& pDesc,
                                        ConstData_t positive,
                                        const TensorDescriptor& nDesc,
                                        ConstData_t negative,
                                        const TensorDescriptor& oDesc,
                                        Data_t o,
                                        float margin,
                                        int p,
                                        float eps,
                                        bool swap,
                                        float divisor);

size_t GetTripletMarginLossBackwardWorkspaceSize(Handle& handle,
                                                 const TensorDescriptor& aDesc,
                                                 const TensorDescriptor& dODesc);

miopenStatus_t TripletMarginLossBackward(Handle& handle,
                                         Data_t workspace,
                                         size_t workspaceSizeInBytes,
                                         const TensorDescriptor& aDesc,
                                         ConstData_t anchor,
                                         const TensorDescriptor& pDesc,
                                         ConstData_t positive,
                                         const TensorDescriptor& nDesc,
                                         ConstData_t negative,
                                         const TensorDescriptor& dODesc,
                                         ConstData_t dO,
                                         const TensorDescriptor& dADesc,
                                         Data_t dA,
                                         const TensorDescriptor& dPDesc,
                                         Data_t dP,
                                         const TensorDescriptor& dNDesc,
                                         Data_t dN,
                                         float margin,
                                         int p,
                                         float eps,
                                         bool swap,
                                         float divisor);

} // namespace miopen
