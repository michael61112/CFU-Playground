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
#include "playground_util/print_params.h"

#include "cfu.h"
#include "perf.h"
#include "models/my_cycles.h"
extern long long unsigned my_cycles;


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
  printf("\n");
  print_conv_params(params, input_shape, filter_shape, output_shape);
#ifdef USE_LITERAL_VALUE
  // Get parameters.
  const int32_t input_offset = params.input_offset;  // r = s(q - Z)
  const int stride_width = 1;//params.stride_width;
  const int stride_height = 1;//params.stride_height;
  const int dilation_width_factor = 1;//params.dilation_width_factor;
  const int dilation_height_factor = params.dilation_height_factor;
  const int pad_width = 0;//params.padding_values.width;
  const int pad_height = 0;//params.padding_values.height;
  const int32_t output_offset = -128;//params.output_offset;
#else
  // Get parameters.
  const int32_t input_offset = params.input_offset;  // r = s(q - Z)
  const int stride_width = params.stride_width;
  const int stride_height = params.stride_height;
  const int dilation_width_factor = params.dilation_width_factor;
  const int dilation_height_factor = params.dilation_height_factor;
  const int pad_width = params.padding_values.width;
  const int pad_height = params.padding_values.height;
  const int32_t output_offset = params.output_offset;
  /*
  printf("input_offset: %ld\n",input_offset);
  printf("stride_width: %d\n",stride_width);
  printf("stride_height: %d\n",stride_height);
  printf("dilation_width_factor: %d\n",dilation_width_factor);
  printf("dilation_height_factor: %d\n",dilation_height_factor);
  printf("pad_width: %d\n",pad_width);
  printf("pad_height: %d\n",pad_height);
  printf("output_offset: %ld\n",output_offset);
  */
#endif
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
#ifdef USE_SIMD
	  int32_t acc = cfu_op0(/* funct7= */ 1, 0, 0); // resets acc
#else
	  int32_t acc = 0;
#endif
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
	      unsigned my_start = perf_get_mcycle();
              perf_enable_counter(0);
#ifdef USE_ORIGIN_MODE              
	      for (int in_channel = 0; in_channel < filter_input_depth; in_channel++) {
		int32_t input_val = input_data[Offset(input_shape, batch, in_y, in_x,
                                      in_channel + group * filter_input_depth)];
                int32_t filter_val = filter_data[Offset(
                    filter_shape, out_channel, filter_y, filter_x, in_channel)];

		acc += filter_val * (input_val + input_offset);
	      }
#elif USE_UNROLLING
	      if (filter_input_depth < 4) {
                for (int in_channel = 0; in_channel < filter_input_depth; in_channel++) {
                  int32_t input_val = input_data[Offset(input_shape, batch, in_y, in_x,
                                      in_channel + group * filter_input_depth)];
                  int32_t filter_val = filter_data[Offset(
                      filter_shape, out_channel, filter_y, filter_x, in_channel)];

                  acc += filter_val * (input_val + input_offset);
                }
	      }
	      else {

                for (int in_channel = 0; in_channel < filter_input_depth; in_channel += 4) {
//--------------------------------------------------------------------------------------------
                    int32_t input_val = input_data[Offset(input_shape, batch, in_y, in_x,
                                          (in_channel + 0) + group * filter_input_depth)];
                    int32_t filter_val = filter_data[Offset(
                               filter_shape, out_channel, filter_y, filter_x, (in_channel + 0))];

                    acc += filter_val * (input_val + input_offset);

//--------------------------------------------------------------------------------------------
                    input_val = input_data[Offset(input_shape, batch, in_y, in_x,
                                          (in_channel + 1) + group * filter_input_depth)];
                    filter_val = filter_data[Offset(
                        filter_shape, out_channel, filter_y, filter_x, (in_channel + 1))];

                    acc += filter_val * (input_val + input_offset);

//--------------------------------------------------------------------------------------------
                    input_val = input_data[Offset(input_shape, batch, in_y, in_x,
                                         (in_channel + 2) + group * filter_input_depth)];
                    filter_val = filter_data[Offset(
                        filter_shape, out_channel, filter_y, filter_x, in_channel + 2)];

        	    acc += filter_val * (input_val + input_offset);
//--------------------------------------------------------------------------------------------
                    input_val = input_data[Offset(input_shape, batch, in_y, in_x,
                                         (in_channel + 3)+ group * filter_input_depth)];
                    filter_val = filter_data[Offset(
                        filter_shape, out_channel, filter_y, filter_x, in_channel + 3)];

                    acc += filter_val * (input_val + input_offset);
                }
	     }
#elif USE_SIMD
             if (filter_input_depth < 4) {
                for (int in_channel = 0; in_channel < filter_input_depth; in_channel++) {
                  int32_t input_val = input_data[Offset(input_shape, batch, in_y, in_x,
                                      in_channel + group * filter_input_depth)];
                  int32_t filter_val = filter_data[Offset(
                      filter_shape, out_channel, filter_y, filter_x, in_channel)];

                  acc += filter_val * (input_val + input_offset);
                }
              }
              else {
 
                for (int in_channel = 0; in_channel < filter_input_depth; in_channel += 4) {
//--------------------------------------------------------------------------------------------
/*			int32_t input_val_temp[4], filter_val_temp[4];
			input_val_temp[0] = input_data[Offset(input_shape, batch, in_y, in_x,
                                        in_channel + group * filter_input_depth + 0)];
         		filter_val_temp[0] = filter_data[Offset(
        		    filter_shape, out_channel, filter_y, filter_x, in_channel + 0)];
//--------------------------------------------------------------------------------------------
		        input_val_temp[1] = input_data[Offset(input_shape, batch, in_y, in_x,
                                      in_channel + group * filter_input_depth + 1)];
		        filter_val_temp[1] = filter_data[Offset(
		            filter_shape, out_channel, filter_y, filter_x, in_channel + 1)];
//--------------------------------------------------------------------------------------------
		        input_val_temp[2] = input_data[Offset(input_shape, batch, in_y, in_x,
                                      in_channel + group * filter_input_depth + 2)];
		        filter_val_temp[2] = filter_data[Offset(
		            filter_shape, out_channel, filter_y, filter_x, in_channel + 2)];
//--------------------------------------------------------------------------------------------
		        input_val_temp[3] = input_data[Offset(input_shape, batch, in_y, in_x,
                                      in_channel + group * filter_input_depth + 3)];
		        filter_val_temp[3] = filter_data[Offset(
		            filter_shape, out_channel, filter_y, filter_x, in_channel + 3)];
//--------------------------------------------------------------------------------------------
			int32_t input_val = 0;
			int32_t filter_val = 0;
			for (int i = 0; i < 4; i++) {
				input_val |= ((input_val_temp[i] + input_offset) & 0xff) << ((3-i)*8);
			}

			for (int i = 0; i < 4; i++) {
                                filter_val |= (filter_val_temp[i] & 0xff) << ((3-i)*8);
                        }
*/
/*
			printf("in0: %ld, %ld\n", (input_val_temp[0] + input_offset), (input_val_temp[0] + input_offset)<<24);
			printf("in1: %ld, %ld\n", (input_val_temp[1] + input_offset), (input_val_temp[1] + input_offset)<<16);
			printf("in2: %ld, %ld\n", (input_val_temp[2] + input_offset), (input_val_temp[2] + input_offset)<<8);
			printf("in3: %ld, %ld\n", (input_val_temp[3] + input_offset), (input_val_temp[3] + input_offset)<<0);
			printf("in_sum: %ld\n", input_val);
			printf("fil0: %ld, %ld\n", filter_val_temp[0], filter_val_temp[0]<<24);
			printf("fil1: %ld, %ld\n", filter_val_temp[1], filter_val_temp[1]<<16);
			printf("fil2: %ld, %ld\n", filter_val_temp[2], filter_val_temp[2]<<8);
			printf("fil3: %ld, %ld\n", filter_val_temp[3], filter_val_temp[3]<<0);
			printf("filter_sum: %ld\n", filter_val);

*/
			uint32_t input_val = *((uint32_t *)(input_data + Offset(
		                input_shape, batch, in_y, in_x, in_channel + group * filter_input_depth)));

            		uint32_t filter_val = *((uint32_t *)(filter_data + Offset(
                		filter_shape, out_channel, filter_y, filter_x, in_channel)));
			acc = cfu_op0(/* funct7= */ 0, /* in0= */ input_val, /* in1= */ filter_val);
//			printf("acc: %ld\n\n", acc);
	          }
	      }
#endif
	      perf_enable_counter(0);
	      unsigned my_finish = perf_get_mcycle();
	      my_cycles += (my_finish - my_start);
            }
          }

//-------------------------------------------------------------------------------------


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
  }
}

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
  //print_conv_params(params, input_shape, filter_shape, output_shape);
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
