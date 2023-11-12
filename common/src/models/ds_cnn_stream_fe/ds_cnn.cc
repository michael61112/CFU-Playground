#include "models/ds_cnn_stream_fe/ds_cnn.h"
#include <stdio.h>
#include "menu.h"
#include "models/ds_cnn_stream_fe/ds_cnn_stream_fe.h"
#include "tflite.h"
#include "models/label/label0_board.h"
#include "models/label/label1_board.h"
#include "models/label/label6_board.h"
#include "models/label/label8_board.h"
#include "models/label/label11_board.h"
#include "models/my_cycles.h"

// Initialize everything once
// deallocate tensors when done
static void ds_cnn_stream_fe_init(void) {
  tflite_load_model(ds_cnn_stream_fe, ds_cnn_stream_fe_len);
}



// Implement your design here
///////////////////////////////////////////////////////
//
//
//
typedef struct {
  uint32_t score0;  // Stored as uint32_t because we can't print floats.
  uint32_t score1;
  uint32_t score2;
  uint32_t score3;
  uint32_t score4;
  uint32_t score5;
  uint32_t score6;
  uint32_t score7;
  uint32_t score8;
  uint32_t score9;
  uint32_t score10;
  uint32_t score11;
} ScoreResult;


ScoreResult kws2_classify() {
  printf("Running KWS\n");
  tflite_classify();


#ifndef USE_SIMD
  printf("No SIMD\n");
#else
  printf("Use SIMD\n");
#endif

#ifndef USE_UNROLLING
  printf("No Use Unrolling\n");
#else
  printf("Use Unrolling\n");
#endif

  // Process the inference results.
  float* output = tflite_get_output_float();

  // Kindly ask for the raw bits of the floats.
  return (ScoreResult){
      *(uint32_t*)&output[0],
      *(uint32_t*)&output[1],
      *(uint32_t*)&output[2],
      *(uint32_t*)&output[3],
      *(uint32_t*)&output[4],
      *(uint32_t*)&output[5],
      *(uint32_t*)&output[6],
      *(uint32_t*)&output[7],
      *(uint32_t*)&output[8],
      *(uint32_t*)&output[9],
      *(uint32_t*)&output[10],
      *(uint32_t*)&output[11],
  };

}

static void print_kws_result(ScoreResult res) {
  printf("0 : 0x%lx, \n1 : 0x%lx, \n2 : 0x%lx, \n3 : 0x%lx, \n4 : 0x%lx, \n5 : 0x%lx, \n", res.score0, res.score1, res.score2, res.score3, res.score4, res.score5);
  printf("6 : 0x%lx, \n7 : 0x%lx, \n8 : 0x%lx, \n9 : 0x%lx, \n10 : 0x%lx, \n11 : 0x%lx, \n", res.score6, res.score7, res.score8, res.score9, res.score10, res.score11);
}
///////////////////////////////////////////////////////////////

// Run classification, after input has been loaded
static int32_t pdti8_classify() {
  printf("Running pdti8\n");
  tflite_classify();

  // Process the inference results.
  int8_t* output = tflite_get_output();
  return output[1] - output[0];
}

static void func_pdti8() {
  tflite_set_input_zeros();
  int32_t result = pdti8_classify();
  printf("  result is %ld\n", result);
  printf("MAC: %lld\n", get_my_cycles());
}
///////////////////////////////////////////////////////
//
//
/*static float kws_classify() {
  printf("Running kws\n");
  tflite_classify();

  // Process the inference results.
  float* output = tflite_get_output_float();
  return output[1] - output[0];
}
*/
static void func_lab_1() {
  tflite_set_input_float(label1_data);
  print_kws_result(kws2_classify());
  printf("MAC: %lld\n", get_my_cycles());
}

static void func_lab_8() {
  tflite_set_input_float(label8_data);
  print_kws_result(kws2_classify());
  printf("MAC: %lld\n", get_my_cycles());
}

///////////////////////////////////////////////////////
//



static void func_run_total_label() {
  puts("Classify kws by label0");
  tflite_set_input_float(label0_data);
  print_kws_result(kws2_classify());
  printf("MAC: %lld\n", get_my_cycles());

  puts("Classify kws by label1");
  tflite_set_input_float(label1_data);
  print_kws_result(kws2_classify());
  printf("MAC: %lld\n", get_my_cycles());

  puts("Classify kws by label6");
  tflite_set_input_float(label6_data);
  print_kws_result(kws2_classify());
  printf("MAC: %lld\n", get_my_cycles());

  puts("Classify kws by label8");
  tflite_set_input_float(label8_data);
  print_kws_result(kws2_classify());
  printf("MAC: %lld\n", get_my_cycles());

  puts("Classify kws by label11");
  tflite_set_input_float(label11_data);
  print_kws_result(kws2_classify());
  printf("MAC: %lld\n", get_my_cycles());
}


static struct Menu MENU = {
    "Tests for ds_cnn_stream_fe",
    "ds_cnn_stream_fe",
    {
	MENU_ITEM('0', "pdti8", func_pdti8),
	MENU_ITEM('1', "label 1", func_lab_1),
	MENU_ITEM('2', "label 8", func_lab_8),
	MENU_ITEM('3', "Run total label", func_run_total_label),
//	MENU_ITEM(AUTO_INC_CHAR, "label 11", func_lab_11),
        MENU_END,
    },
};

// For integration into menu system
void ds_cnn_stream_fe_menu() {
  ds_cnn_stream_fe_init();
  menu_run(&MENU);
}


