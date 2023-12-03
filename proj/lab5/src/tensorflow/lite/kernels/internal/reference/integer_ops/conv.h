/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef TENSORFLOW_LITE_KERNELS_INTERNAL_REFERENCE_INTEGER_OPS_CONV_H_
#define TENSORFLOW_LITE_KERNELS_INTERNAL_REFERENCE_INTEGER_OPS_CONV_H_

#include <algorithm>

#include "tensorflow/lite/kernels/internal/common.h"
#include "tensorflow/lite/kernels/internal/portable_tensor_utils.h"
#include <cstdio>

#include "cfu.h"
#include "perf.h"
#include "models/my_cycles.h"

extern long long unsigned my_cycles;
/*
int32_t matrix_fmaps[65536][65536];
int32_t matrix_filter[65536][65536];
int32_t matrix_result[65536][65536];
*/
int32_t matrix_fmaps[1024][1024];
int32_t matrix_filter[1024][1024];
int32_t matrix_result[1024][1024];

void matrix_multiply(int32_t* A, int rowsA, int colsA, int32_t* B, int rowsB, int colsB, int32_t* result) {
    // Check if matrix dimensions are compatible for multiplication
    if (colsA != rowsB) {
        printf("Error: Incompatible matrix dimensions for multiplication\n");
        return;
    }

    // Perform matrix multiplication
    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            result[i * colsB + j] = 0;
            for (int k = 0; k < colsA; ++k) {
                result[i * colsB + j] += A[i * colsA + k] * B[k * colsB + j];
            }
        }
    }
}

void matrix_multiply2D(int rowsA, int colsA, int rowsB, int colsB) {
    // Check if matrix dimensions are compatible for multiplication
    if (colsA != rowsB) {
        printf("Error: Incompatible matrix dimensions for multiplication\n");
        return;
    }

    // Perform matrix multiplication
    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            matrix_result[i][j] = 0;
            for (int k = 0; k < colsA; ++k) {
                matrix_result[i][j] += matrix_fmaps[i][k] * matrix_filter[k][j];
            }
        }
    }
}

void matrix_multiply2D_acc(int baseM, int baseN, int baseK, int blockSize) {
  int K = blockSize;
  int M = blockSize;
  int N = blockSize;

  //--------------------------------------------------

  cfu_op0(1, 0, 0);  // reset
  cfu_op0(1, 1, 0);
  //--------------------------------------------------
  cfu_op0(/* funct7= */ 2, /* in0= */ K, /* in1= */ K);  // Set parameter K
  cfu_op0(/* funct7= */ 4, /* in0= */ M, /* in1= */ M);  // Set parameter M
  cfu_op0(/* funct7= */ 6, /* in0= */ N, /* in1= */ N);  // Set parameter N

  int calignA = int((M + 3) / 4) * 4;
  int16_t addr = 0;
  for (int dr = baseM; dr < baseM + calignA; dr += 4) {
    for (int cptr = baseK; cptr < baseK + K; cptr += 1) {
      int32_t in_data4 = 0;

      int32_t a0 = 0, a1 = 0, a2 = 0, a3 = 0;
      // if K M >4
      if ((dr-baseM) < K) {
        a3 = matrix_fmaps[dr + 3][cptr];
        a2 = matrix_fmaps[dr + 2][cptr];
        a1 = matrix_fmaps[dr + 1][cptr];
        a0 = matrix_fmaps[dr + 0][cptr];
      } else {
        switch (K % 4) {
          case 1:
            a3 = 0;
            a2 = 0;
            a1 = 0;
            a0 = matrix_fmaps[dr + 0][cptr];
            break;
          case 2:
            a3 = 0;
            a2 = 0;
            a1 = matrix_fmaps[dr + 1][cptr];
            a0 = matrix_fmaps[dr + 0][cptr];
            break;
          case 3:
            a3 = 0;
            a2 = matrix_fmaps[dr + 2][cptr];
            a1 = matrix_fmaps[dr + 1][cptr];
            a0 = matrix_fmaps[dr + 0][cptr];
            break;
        }
      }
      in_data4 |= (a3 & 0xFF);
      in_data4 |= ((int32_t)(a2 & 0xFF) << 8);
      in_data4 |= ((int32_t)(a1 & 0xFF) << 16);
      in_data4 |= ((int32_t)(a0 & 0xFF) << 24);

      //printf("%ld\t%ld\t%ld\t%ld\n", a0, a1, a2, a3);
      cfu_op0(8, addr, in_data4);  // Set global bufer A
      addr++;
    }
  }

  int calignB = int((N + 3) / 4) * 4;
  addr = 0;
  for (int cptr = baseN; cptr < baseN + calignB; cptr += 4) {
    for (int dr = baseK; dr < baseK + K; dr++) {
      int32_t in_data4 = 0;
      int32_t b0 = 0, b1 = 0, b2 = 0, b3 = 0;

      if ((cptr-baseN) < N) {
        b3 = matrix_filter[dr][cptr + 3];
        b2 = matrix_filter[dr][cptr + 2];
        b1 = matrix_filter[dr][cptr + 1];
        b0 = matrix_filter[dr][cptr + 0];
      } else {
        switch (N % 4) {
          case 1:
            b3 = 0;
            b2 = 0;
            b1 = 0;
            b0 = matrix_filter[dr][cptr + 0];
            break;
          case 2:
            b3 = 0;
            b2 = 0;
            b1 = matrix_filter[dr][cptr + 1];
            b0 = matrix_filter[dr][cptr + 0];
            break;
          case 3:
            b3 = 0;
            b2 = matrix_filter[dr][cptr + 2];
            b1 = matrix_filter[dr][cptr + 1];
            b0 = matrix_filter[dr][cptr + 0];
            break;
        }
      }
      in_data4 |= (b3 & 0xFF);
      in_data4 |= ((int32_t)(b2 & 0xFF) << 8);
      in_data4 |= ((int32_t)(b1 & 0xFF) << 16);
      in_data4 |= ((int32_t)(b0 & 0xFF) << 24);
      //printf("%ld\t%ld\t%ld\t%ld\n", b0, b1, b2, b3);

      cfu_op0(10, addr, in_data4);  // Set global bufer B
      addr++;
    }
  }
  //--------------------------------------------------
  //printf("In valid\n");
  cfu_op0(12, 0, 0);
  //--------------------------------------------------
  // Check Status
  while (1) {
    int busy = cfu_op0(13, 0, 0);
    if (!busy) break;
  }

  int calignC = int((N + 3) / 4) * 4;
  addr = 0;
  for (int cptr = baseN; cptr < baseN + calignC; cptr += 4) {
    for (int dr = baseM; dr < baseM + M; dr++) {

      matrix_result[dr][cptr + 3] += cfu_op0(14, addr, 0);
      matrix_result[dr][cptr + 2] += cfu_op0(15, addr, 0);
      matrix_result[dr][cptr + 1] += cfu_op0(16, addr, 0);
      matrix_result[dr][cptr + 0] += cfu_op0(17, addr, 0);
      addr++;
    }
  }
}

namespace tflite {
namespace reference_integer_ops {

// Fixed-point per-channel-quantization convolution reference kernel.
inline void ConvPerChannel(
    const ConvParams& params, const int32_t* output_multiplier,
    const int32_t* output_shift, const RuntimeShape& input_shape,
    const int8_t* input_data, const RuntimeShape& filter_shape,
    const int8_t* filter_data, const RuntimeShape& bias_shape,
    const int32_t* bias_data, const RuntimeShape& output_shape,
    int8_t* output_data) {
  // Get parameters.
  const int32_t input_offset = params.input_offset;  // r = s(q - Z)
  const int stride_width = params.stride_width;
  const int stride_height = params.stride_height;
  const int dilation_width_factor = params.dilation_width_factor;
  const int dilation_height_factor = params.dilation_height_factor;
  const int pad_width = params.padding_values.width;
  const int pad_height = params.padding_values.height;
  const int32_t output_offset = params.output_offset;

  // Set min and max value of the output.
  const int32_t output_activation_min = params.quantized_activation_min;
  const int32_t output_activation_max = params.quantized_activation_max;

  // Consistency check.
  TFLITE_DCHECK_LE(output_activation_min, output_activation_max);
  TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);
  const int batches = MatchingDim(input_shape, 0, output_shape, 0);
  const int input_depth = input_shape.Dims(3);
  const int output_depth = MatchingDim(filter_shape, 0, output_shape, 3);
  if (bias_data) {
    TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);
  }

  // Check dimensions of the tensors.
  const int input_height = input_shape.Dims(1);
  const int input_width = input_shape.Dims(2);
  const int filter_height = filter_shape.Dims(1);
  const int filter_width = filter_shape.Dims(2);
  const int filter_input_depth = filter_shape.Dims(3);
  const int groups = input_depth / filter_input_depth;
  TFLITE_DCHECK_EQ(input_depth % filter_input_depth, 0);
  const int filters_per_group = output_depth / groups;
  const int output_height = output_shape.Dims(1);
  const int output_width = output_shape.Dims(2);
/*
  printf("input_depth: %d\n",input_depth);
  printf("filter_input_depth: %d\n",filter_input_depth);
  printf("output_depth: %d\n",output_depth);
  printf("groups: %d\n",groups);
  printf("filters_per_group: %d\n",filters_per_group);
*/


  // fmaps_num * fmaps_size
  int fmaps_num = output_height * output_width;
  int fmaps_size = filter_height * filter_width * input_depth;

  // filter_size * filter_num
  int filter_size = filter_height * filter_width * filter_input_depth;
  int filter_num = output_depth;

  // result_size * result_channel
  int result_size = fmaps_num;
  int result_num = filter_num;

  printf("\n\n");
  printf("fmaps_num: %d\n",fmaps_num);
  printf("fmaps_size: %d\n",fmaps_size);
  printf("filter_size: %d\n",filter_size);
  printf("filter_num: %d\n",filter_num);
  printf("result_size: %d\n", result_size);
  printf("result_num: %d\n",result_num);

  int32_t acc;

  int fmaps_row;
  int fmaps_col;

  int filter_row;
  int filter_col;

  int result_row;
  int result_col;

  for (int batch = 0; batch < batches; ++batch)
    for (int out_y = 0; out_y < output_height; ++out_y)
      for (int out_x = 0; out_x < output_width; ++out_x)
        for (int out_channel = 0; out_channel < output_depth; ++out_channel) {
          output_data[Offset(output_shape, batch, out_y, out_x, out_channel)] =
		  static_cast<int8_t> (0);
	}


  for (int batch = 0; batch < batches; ++batch) {

unsigned my_start = perf_get_mcycle();
//-------------------------------------------------------------------------------
    
//  fmaps prepare : 3D to 2D

    for (int out_y = 0; out_y < output_height; ++out_y) {
      const int in_y_origin = (out_y * stride_height) - pad_height;
      for (int out_x = 0; out_x < output_width; ++out_x) {
        const int in_x_origin = (out_x * stride_width) - pad_width;       
        for (int out_channel = 0; out_channel < output_depth; ++out_channel) {
          auto group = out_channel / filters_per_group;
//----
          for (int filter_y = 0; filter_y < filter_height; ++filter_y) {
            const int in_y = in_y_origin + dilation_height_factor * filter_y;
            for (int filter_x = 0; filter_x < filter_width; ++filter_x) {
              const int in_x = in_x_origin + dilation_width_factor * filter_x;
//-----
              // Zero padding by omitting the areas outside the image.
              const bool is_point_inside_image = (in_x >= 0) && (in_x < input_width) && (in_y >= 0) && (in_y < input_height);

              if (!is_point_inside_image) {
                continue;
              }

	      for (int in_channel = 0; in_channel < filter_input_depth; ++in_channel) {

	        int32_t input_val = (int32_t)input_data[Offset(input_shape, batch, in_y, in_x, in_channel + group * filter_input_depth)] + input_offset;
    
	        fmaps_row = out_x + out_y * output_width; //fmaps_num // location index
	        fmaps_col = filter_x + filter_y * filter_width  + in_channel * (filter_height * filter_width); //fmaps_size //content * channel
          /*
          if (input_val > -128) {
            if (input_val < 127)
              input_val = input_val;
            else
              input_val = 127;
          }
          else {
            input_val = -128;
          }
          */
          //if (input_val>127 || input_val < -128)
            //printf("i:%d, j:%d, input_val=%ld, intput_val int8_t=%d, offset =%ld\n",fmaps_row,fmaps_col,input_val,input_data[Offset(input_shape, batch, in_y, in_x, in_channel + group * filter_input_depth)], input_offset);

	        //matrix_fmaps[fmaps_row * fmaps_size + fmaps_col] = input_val;
		matrix_fmaps[fmaps_row][fmaps_col] = input_val;
              }
            }
          }
	}
      }
    }


//----------------------------------------------------------------------------------------------------
//  filter prepare : 4D to 2D

for (int out_channel = 0; out_channel < output_depth; ++out_channel) {
  for (int filter_y = 0; filter_y < filter_height; ++filter_y) {
    for (int filter_x = 0; filter_x < filter_width; ++filter_x) {
      for (int in_channel = 0; in_channel < filter_input_depth; ++in_channel) {
        int32_t filter_val = (int32_t)filter_data[Offset(filter_shape, out_channel, filter_y, filter_x, in_channel)];

	filter_row = filter_x + filter_y * filter_width + in_channel * (filter_height * filter_width); // filter_size
	filter_col = out_channel; // filter_num

	//matrix_filter[filter_row * filter_num + filter_col] = filter_val;
	matrix_filter[filter_row][filter_col] = filter_val;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------

//  matrix_multiply(matrix_fmaps, fmaps_num, fmaps_size, matrix_filter, filter_size, filter_num, matrix_result);
  //matrix_multiply2D( fmaps_num, fmaps_size,  filter_size, filter_num);


  int K = fmaps_size;
  int M = fmaps_num;
  int N = filter_num;

  int blockSize = 8;
  int KK = int((K + (blockSize-1))/blockSize)*blockSize;
  int MM = int((M + (blockSize-1))/blockSize)*blockSize;
  int NN = int((N + (blockSize-1))/blockSize)*blockSize;


//-------------------------------------------
  for (int i = M; i < MM; i++) {
    for (int j = 0; j < KK; j++) {
      matrix_fmaps[i][j] = 0;
    }
  }
  for (int i = 0; i < MM; i++) {
    for (int j = K; j < KK; j++) {
      matrix_fmaps[i][j] = 0;
    }
  }

  for (int i = K; i < KK; i++) {
    for (int j = 0; j < NN; j++) {
      matrix_filter[i][j] = 0;
    }
  }
  for (int i = 0; i < KK; i++) {
    for (int j = N; j < NN; j++) {
      matrix_filter[i][j] = 0;
    }
  }
  // Clear output matrix of blockSize matrix multiply acc
  for (int i = 0; i < MM; i++) {
    for (int j = 0; j < NN; j++) {
      matrix_result[i][j] = 0;
    }
  }

  for (int ii = 0; ii < MM; ii+=blockSize)
    for (int jj = 0; jj < NN; jj+=blockSize)
      for (int kk = 0; kk < KK; kk+=blockSize)
        matrix_multiply2D_acc(ii, jj, kk, blockSize);

//----------------------------------------------------------------------------------------------------


  for (int out_channel = 0; out_channel < output_depth; ++out_channel) {
    for (int out_y = 0; out_y < output_height; ++out_y) {
      for (int out_x = 0; out_x < output_width; ++out_x) {
        
	
          result_row = out_x + out_y * output_width; // result_size
	  result_col = out_channel; // result_num

//	  acc = matrix_result[result_row * result_num + result_col];
	  acc = matrix_result[result_row][result_col];

	  if (bias_data) {
            acc += bias_data[out_channel];
          }
          acc = MultiplyByQuantizedMultiplier(
              acc, output_multiplier[out_channel], output_shift[out_channel]);
          acc += output_offset;
          acc = std::max(acc, output_activation_min);
          acc = std::min(acc, output_activation_max);
          output_data[Offset(output_shape, batch, out_y, out_x, out_channel)] =
              static_cast<int8_t>(acc);
        }
      }
    }

for(int i=0;i<4;i++){
  for(int j=0;j<4;j++){
    printf("%ld\t",matrix_result[i][j]);
  }
  printf("\n");
}

printf("check matric A\n");
for(int i=0;i<M;i++){
  for(int j=0;j<K;j++){
    int32_t t = matrix_fmaps[i][j];
    if (t>127 || t < -128)
    printf("i:%d, j:%d, buffer A=%ld\n",i,j,t);
  }
  //printf("\n");
}
printf("check matric B\n");
for(int i=0;i<K;i++){
  for(int j=0;j<N;j++){
    int32_t t = matrix_filter[i][j];
    if (t>127 || t < -128)
    printf("i:%d, j:%d, buffer B=%ld\n",i,j,t);
  }
  //printf("\n");
}
/*
printf("check matric C");
for(int i=0;i<M;i++){
  for(int j=0;j<N;j++){
    int32_t t = matrix_result[i][j];
    if (t>2147483647 || t < -2147483648)
    printf("i:%d, j:%d, buffer C=%ld\n",i,j,t);
  }
  printf("\n");
}*/

unsigned my_finish = perf_get_mcycle();
my_cycles += (my_finish - my_start);
  } // batch
}  // ConvPerChannel

inline void ConvPerChannelWithPackedInt4Weights(
    const ConvParams& params, const int32_t* output_multiplier,
    const int32_t* output_shift, const RuntimeShape& input_shape,
    const int8_t* input_data, const RuntimeShape& filter_shape,
    const int8_t* filter_input, int8_t* unpacked_filter_data,
    const RuntimeShape& bias_shape, const int32_t* bias_data,
    const RuntimeShape& output_shape, int8_t* output_data) {
  TFLITE_DCHECK(unpacked_filter_data != nullptr);
  tflite::tensor_utils::UnpackDenseInt4IntoInt8(
      filter_input, filter_shape.FlatSize(), unpacked_filter_data);
  ConvPerChannel(params, output_multiplier, output_shift, input_shape,
                 input_data, filter_shape, unpacked_filter_data, bias_shape,
                 bias_data, output_shape, output_data);
}

// Fixed-point per-channel-quantization convolution reference kernel.
// 16-bit data and 8-bit filter
template <typename AccumScalar>
inline void ConvPerChannel(
    const ConvParams& params, const int32_t* output_multiplier,
    const int32_t* output_shift, const RuntimeShape& input_shape,
    const int16_t* input_data, const RuntimeShape& filter_shape,
    const int8_t* filter_data, const RuntimeShape& bias_shape,
    const AccumScalar* bias_data, const RuntimeShape& output_shape,
    int16_t* output_data) {
  // Get parameters.
  const int stride_width = params.stride_width;
  const int stride_height = params.stride_height;
  const int dilation_width_factor = params.dilation_width_factor;
  const int dilation_height_factor = params.dilation_height_factor;
  const int pad_width = params.padding_values.width;
  const int pad_height = params.padding_values.height;

  // Set min and max value of the output.
  const int32_t output_activation_min = params.quantized_activation_min;
  const int32_t output_activation_max = params.quantized_activation_max;

  // Consistency check.
  TFLITE_DCHECK_LE(output_activation_min, output_activation_max);
  TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);
  const int batches = MatchingDim(input_shape, 0, output_shape, 0);
  const int input_depth = input_shape.Dims(3);
  const int output_depth = MatchingDim(filter_shape, 0, output_shape, 3);
  if (bias_data) {
    TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);
  }

  // Check dimensions of the tensors.
  const int input_height = input_shape.Dims(1);
  const int input_width = input_shape.Dims(2);
  const int filter_height = filter_shape.Dims(1);
  const int filter_width = filter_shape.Dims(2);
  const int filter_input_depth = filter_shape.Dims(3);
  const int groups = input_depth / filter_input_depth;
  TFLITE_DCHECK_EQ(input_depth % filter_input_depth, 0);
  const int filters_per_group = output_depth / groups;
  const int output_height = output_shape.Dims(1);
  const int output_width = output_shape.Dims(2);
  for (int batch = 0; batch < batches; ++batch) {
    for (int out_y = 0; out_y < output_height; ++out_y) {
      const int in_y_origin = (out_y * stride_height) - pad_height;
      for (int out_x = 0; out_x < output_width; ++out_x) {
        const int in_x_origin = (out_x * stride_width) - pad_width;
        for (int out_channel = 0; out_channel < output_depth; ++out_channel) {
          auto group = out_channel / filters_per_group;
          AccumScalar acc = 0;
          for (int filter_y = 0; filter_y < filter_height; ++filter_y) {
            const int in_y = in_y_origin + dilation_height_factor * filter_y;
            for (int filter_x = 0; filter_x < filter_width; ++filter_x) {
              const int in_x = in_x_origin + dilation_width_factor * filter_x;

              // Zero padding by omitting the areas outside the image.
              const bool is_point_inside_image =
                  (in_x >= 0) && (in_x < input_width) && (in_y >= 0) &&
                  (in_y < input_height);

              if (!is_point_inside_image) {
                continue;
              }

              for (int in_channel = 0; in_channel < filter_input_depth;
                   ++in_channel) {
                int32_t input_val =
                    input_data[Offset(input_shape, batch, in_y, in_x,
                                      in_channel + group * filter_input_depth)];
                int32_t filter_val = filter_data[Offset(
                    filter_shape, out_channel, filter_y, filter_x, in_channel)];
                // Accumulate with 64 bits accumulator.
                // int64_t += int8_t * int16_t so the highest value we can
                // get from each accumulation is [-127, 127] * ([-32768,
                // 32767] -
                // [-32768, 32767]), which is [-8322945, 8322945].
                // log2(8322945) = 22.99.
                acc += filter_val * input_val;
              }
            }
          }
          if (bias_data) {
            acc += bias_data[out_channel];
          }
          int32_t scaled_acc = MultiplyByQuantizedMultiplier(
              acc, output_multiplier[out_channel], output_shift[out_channel]);
          scaled_acc = std::max(scaled_acc, output_activation_min);
          scaled_acc = std::min(scaled_acc, output_activation_max);
          output_data[Offset(output_shape, batch, out_y, out_x, out_channel)] =
              static_cast<int16_t>(scaled_acc);
        }
      }
    }
  }
}

}  // namespace reference_integer_ops
}  // namespace tflite

#endif  // TENSORFLOW_LITE_KERNELS_INTERNAL_REFERENCE_INTEGER_OPS_CONV_H_
