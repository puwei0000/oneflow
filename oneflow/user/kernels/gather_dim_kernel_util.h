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
#ifndef ONEFLOW_USER_KERNELS_ND_INDEX_SLICE_UTIL_H_
#define ONEFLOW_USER_KERNELS_ND_INDEX_SLICE_UTIL_H_
#include "oneflow/core/kernel/util/cuda_kernel_util.h"
#include "oneflow/core/ndarray/xpu_util.h"

namespace oneflow {

#define MAX_DIM_COUNT 8
/**
 * @brief The converter of one dimension offset and high dimension coordinate
 * 
 * @tparam IDXTYPE the type of index (int32_t or int64_t)
 */
template<typename IDXTYPE>
struct CoordinateOffsetConverter {
  static const unsigned int MAX_AXIS = MAX_DIM_COUNT;

  CoordinateOffsetConverter(const ShapeView& tensorShape)
      : offset_(0), axisNum_(tensorShape.NumAxes()) {
    FOR_RANGE(int64_t, i, 0, MAX_AXIS) {
      shape_[i] = 0;
      coordinate_[i] = 0;
    }

    FOR_RANGE(int64_t, i, 0, axisNum_) { shape_[i] = tensorShape.At(i); }

    if (axisNum_ == 1) { shape_[axisNum_] = 1; }
  }

  OF_DEVICE_FUNC void setOffset(IDXTYPE idx) { offset_ = idx; }

  OF_DEVICE_FUNC IDXTYPE coordinateToIdx() {
    offset_ = 0;
    FOR_RANGE(IDXTYPE, i, 0, axisNum_) {
      IDXTYPE tmp = 1;
      for (IDXTYPE j = i + 1; j < axisNum_; j++) { tmp *= shape_[j]; }
      offset_ += tmp * coordinate_[i];
    }

    return offset_;
  }

  OF_DEVICE_FUNC void idxToCoordinate() {
    IDXTYPE tmp = offset_;
    for (IDXTYPE i = axisNum_ - 1; i >= 0; --i) {
      coordinate_[i] = tmp % shape_[i];
      tmp = (tmp - coordinate_[i]) / shape_[i];
    }
  }

  OF_DEVICE_FUNC void copyCoordinate(CoordinateOffsetConverter otherConverter) {
    FOR_RANGE(int64_t, i, 0, axisNum_) { coordinate_[i] = otherConverter.coordinate_[i]; }
  }

  IDXTYPE coordinate_[MAX_AXIS];
  IDXTYPE offset_;
  int64_t axisNum_;
  int64_t shape_[MAX_AXIS];
};

template<typename IN_T, typename IDX_T>
OF_DEVICE_FUNC void DoGatherDim(CoordinateOffsetConverter<IDX_T> input_helper,
                                CoordinateOffsetConverter<IDX_T> index_helper, int64_t elem_cnt,
                                int64_t dim, const IDX_T* index, const IN_T* input, IN_T* output) {
  XPU_1D_KERNEL_LOOP(index_offset, elem_cnt) {
    // get coordinate of index tensor
    index_helper.setOffset(index_offset);
    index_helper.idxToCoordinate();

    // get coordinate of input tensor by replacing "dim" axis
    const IDX_T x = index[index_offset];
    input_helper.copyCoordinate(index_helper);
    input_helper.coordinate_[dim] = x;

    // set output value at index_offset
    IDX_T input_offset = input_helper.coordinateToIdx();
    output[index_offset] = input[input_offset];
  }
}

template<typename IN_T, typename IDX_T>
OF_DEVICE_FUNC void DoScatterDimAdd(CoordinateOffsetConverter<IDX_T> src_helper,
                                    CoordinateOffsetConverter<IDX_T> output_helper,
                                    int64_t elem_cnt, int64_t dim, const IDX_T* index,
                                    const IN_T* src, IN_T* output) {
  XPU_1D_KERNEL_LOOP(src_offset, elem_cnt) {
    // output[index[i][j][k]][j][k] = src[i][j][k]  # if dim == 0
    // index.shape == src.shape

    // get coordinate of src tensor
    src_helper.setOffset(src_offset);
    src_helper.idxToCoordinate();

    // get coordinate of output tensor by replacing "dim" axis
    output_helper.copyCoordinate(src_helper);
    output_helper.coordinate_[dim] = index[src_offset];

    // set output value at index_offset
    IDX_T output_offset = output_helper.coordinateToIdx();
    output[output_offset] += src[src_offset];

  }
}

}  // namespace oneflow

#endif  // ONEFLOW_USER_KERNELS_ND_INDEX_SLICE_UTIL_H_