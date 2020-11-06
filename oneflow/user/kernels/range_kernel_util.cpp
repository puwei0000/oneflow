/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "oneflow/user/kernels/range_kernel_util.h"

namespace oneflow {

namespace user_op{
template<typename T>
struct RangeFunctor<DeviceType::kCPU, T> {
  void Range(DeviceCtx* ctx, const int start, const int delta, const int range_shape,
                    T* out) {
    DoRange<T>(start, delta, range_shape, out);
  }
};

OF_PP_SEQ_PRODUCT_FOR_EACH_TUPLE(INSTANTIATE_RANGE_FUNCTOR, (DeviceType::kCPU),
                                 RANGE_DATA_TYPE_SEQ);

} // namespace user_op
}  // namespace oneflow

