/*
 * Copyright 2021 The CFU-Playground Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "proj_menu.h"

#include <stdio.h>

#include "cfu.h"
#include "menu.h"

namespace {

// Template Fn

void do_hello_world(void) { puts("Hello, World!!!\n"); }

// Test template instruction
void do_grid_cfu_op0(void) {
  puts("\nExercise CFU Op0\n");
  printf("a   b-->");
  for (int b = 0; b < 6; b++) {
    printf("%8d", b);
  }
  puts("\n-------------------------------------------------------");
  for (int a = 0; a < 6; a++) {
    printf("%-8d", a);
    for (int b = 0; b < 6; b++) {
      int cfu = cfu_op0(0, a, b);
      printf("%8d", cfu);
    }
    puts("");
  }
}

// Test template instruction
void do_exercise_cfu_op0(void) {
  puts("\nExercise CFU Op0\n");
  int count = 0;
  for (int a = -0x71234567; a < 0x68000000; a += 0x10012345) {
    for (int b = -0x7edcba98; b < 0x68000000; b += 0x10770077) {
      int cfu = cfu_op0(0, a, b);
      printf("a: %08x b:%08x cfu=%08x\n", a, b, cfu);
      if (cfu != a) {
        printf("\n***FAIL\n");
        return;
      }
      count++;
    }
  }
  printf("Performed %d comparisons", count);
}


void read_write_cfu(void) {
  int K=100;
  int M=200;
  int N=300;
  printf("Reset\n");
  cfu_op0(/* funct7= */ 1, /* in0= */ 0, /* in1= */ 0); // reset
  printf("Reset Done & Set K\n");
  cfu_op0(/* funct7= */ 2, /* in0= */ K, /* in1= */ K); // Set parameter K
  printf("Set K Done & Read K\n");
  int K_ret = cfu_op0(/* funct7= */ 3, /* in0= */ K, /* in1= */ K); // Read parameter K
  printf("Read K Done & Set M\n");
  cfu_op0(/* funct7= */ 4, /* in0= */ M, /* in1= */ M); // Set parameter M
  printf("Set M Done & Read M\n");
  int M_ret = cfu_op0(/* funct7= */ 5, /* in0= */ M, /* in1= */ M); // Set parameter M
  printf("Read M Done & Set N\n");
  cfu_op0(/* funct7= */ 6, /* in0= */ N, /* in1= */ N); // Set parameter N
  printf("Set N Done & Read N\n");
  int N_ret =  cfu_op0(/* funct7= */ 7, /* in0= */ N, /* in1= */ N); // Set parameter N
  printf("Read N Done\n");

  printf ("Set K: %d, Return K: %d\n", K, K_ret);
  printf ("Set M: %d, Return M: %d\n", M, M_ret);
  printf ("Set N: %d, Return N: %d\n", N, N_ret);
}
struct Menu MENU = {
    "Project Menu",
    "project",
    {
        MENU_ITEM('0', "exercise cfu op0", do_exercise_cfu_op0),
        MENU_ITEM('g', "grid cfu op0", do_grid_cfu_op0),
        MENU_ITEM('h', "say Hello", do_hello_world),
        MENU_ITEM('r', "read_write_cfu", read_write_cfu),
        MENU_END,
    },
};

};  // anonymous namespace

extern "C" void do_proj_menu() { menu_run(&MENU); }
