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
  int K = 100;
  int M = 200;
  int N = 300;
  int in_data4 = 256;
  int32_t addr = 333;
  printf("Reset\n");
  cfu_op0(/* funct7= */ 1, /* in0= */ 0, /* in1= */ 0);  // reset
  printf("Reset Done & Set K\n");
  cfu_op0(/* funct7= */ 2, /* in0= */ K, /* in1= */ K);  // Set parameter K
  printf("Set K Done & Read K\n");
  int K_ret =
      cfu_op0(/* funct7= */ 3, /* in0= */ K, /* in1= */ K);  // Read parameter K
  printf("Read K Done & Set M\n");
  cfu_op0(/* funct7= */ 4, /* in0= */ M, /* in1= */ M);  // Set parameter M
  printf("Set M Done & Read M\n");
  int M_ret =
      cfu_op0(/* funct7= */ 5, /* in0= */ M, /* in1= */ M);  // Read parameter M
  printf("Read M Done & Set N\n");
  cfu_op0(/* funct7= */ 6, /* in0= */ N, /* in1= */ N);  // Set parameter N
  printf("Set N Done & Read N\n");
  int N_ret =
      cfu_op0(/* funct7= */ 7, /* in0= */ N, /* in1= */ N);  // Set parameter N
  printf("Read N Done\n");

  printf("Set K: %d, Return K: %d\n", K, K_ret);
  printf("Set M: %d, Return M: %d\n", M, M_ret);
  printf("Set N: %d, Return N: %d\n", N, N_ret);

  cfu_op0(/* funct7= */ 8, /* in0= */ addr,
          /* in1= */ in_data4);  // Set global bufer A
  int32_t ret = cfu_op0(/* funct7= */ 9, /* in0= */ addr,
                        /* in1= */ in_data4);  // Read global bufer A
  printf("Set Buffer A, in: %d, \t\taddr: %lX, \t\tout: %lX\n", in_data4, addr,
         ret);

  cfu_op0(/* funct7= */ 10, /* in0= */ addr + 1000,
          /* in1= */ in_data4 + 1000);  // Set global bufer B
  int32_t ret2 = cfu_op0(/* funct7= */ 11, /* in0= */ addr + 1000,
                         /* in1= */ 256);  // Read global bufer B
  printf("Set Buffer B, in: %d, \t\taddr: %lX, \t\tout: %lX\n", in_data4 + 1000,
         addr + 1000, ret2);
}

int32_t matrix_fmaps[100][100];
int32_t matrix_filter[100][100];
int32_t matrix_result[100][100];

void matrix_multiply2D_acc(int M, int N, int K) {
  //--------------------------------------------------
  printf("Reset TPU\n");
  cfu_op0(1, 0, 0);  // reset
  cfu_op0(1, 1, 0);
  //--------------------------------------------------
  cfu_op0(/* funct7= */ 2, /* in0= */ K, /* in1= */ K);  // Set parameter K
  cfu_op0(/* funct7= */ 4, /* in0= */ M, /* in1= */ M);  // Set parameter M
  cfu_op0(/* funct7= */ 6, /* in0= */ N, /* in1= */ N);  // Set parameter N

  printf("\nMatrix A for global buffer =\n");
  int calignA = int((M + 3) / 4) * 4;
  int16_t addr = 0;
  for (int dr = 0; dr < calignA; dr += 4) {
    for (int cptr = 0; cptr < K; cptr += 1) {
      int32_t in_data4 = 0;

      int32_t a0 = 0, a1 = 0, a2 = 0, a3 = 0;
      // if K M >4
      if (dr < K - 1) {
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

      printf("%ld\t%ld\t%ld\t%ld\n", a0, a1, a2, a3);
      cfu_op0(8, addr, in_data4);  // Set global bufer A
      addr++;
    }
  }
  printf("\nMatrix B for global buffer =\n");
  int calignB = int((N + 3) / 4) * 4;
  addr = 0;
  for (int cptr = 0; cptr < calignB; cptr += 4) {
    for (int dr = 0; dr < K; dr++) {
      int32_t in_data4 = 0;
      int32_t b0 = 0, b1 = 0, b2 = 0, b3 = 0;
      // if M N >4
      if (cptr < N - 1) {
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
      printf("%ld\t%ld\t%ld\t%ld\n", b0, b1, b2, b3);
      cfu_op0(10, addr, in_data4);  // Set global bufer B
      addr++;
    }
  }
  //--------------------------------------------------
  printf("In valid\n");
  cfu_op0(12, 0, 0);
  //--------------------------------------------------
  // Check Status
  while (1) {
    int busy = cfu_op0(13, 0, 0);
    if (!busy) break;
  }

  int calignC = int((N + 3) / 4) * 4;
  addr = 0;
  for (int cptr = 0; cptr < calignC; cptr += 4) {
    for (int dr = 0; dr < M; dr++) {
      matrix_result[dr][cptr + 3] = cfu_op0(14, addr, 0);
      matrix_result[dr][cptr + 2] = cfu_op0(15, addr, 0);
      matrix_result[dr][cptr + 1] = cfu_op0(16, addr, 0);
      matrix_result[dr][cptr + 0] = cfu_op0(17, addr, 0);
      addr++;
    }
  }
}

void matrix_multiply2D_acc_block(int baseM, int baseN, int baseK, int blockSize) {
  int K = blockSize;
  int M = blockSize;
  int N = blockSize;

  //--------------------------------------------------
  printf("Reset TPU\n");
  cfu_op0(1, 0, 0);  // reset
  cfu_op0(1, 1, 0);
  //--------------------------------------------------
  cfu_op0(/* funct7= */ 2, /* in0= */ K, /* in1= */ K);  // Set parameter K
  cfu_op0(/* funct7= */ 4, /* in0= */ M, /* in1= */ M);  // Set parameter M
  cfu_op0(/* funct7= */ 6, /* in0= */ N, /* in1= */ N);  // Set parameter N

  //printf("\nMatrix A for global buffer =\n");
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
  //printf("\nMatrix B for global buffer =\n");
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
  printf("In valid\n");
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

void prepare_matrixAB(int M, int K, int N, bool negtive) {
  printf("\nMatrix A =\n");
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < K; j++) {
      if (negtive & (i%2==0))
        matrix_fmaps[i][j] = -(j + K * i);
      else
        matrix_fmaps[i][j] = j + K * i;
      printf("%ld\t", matrix_fmaps[i][j]);
    }
    printf("\n");
  }
  printf("\nMatrix B =\n");
  for (int i = 0; i < K; i++) {
    for (int j = 0; j < N; j++) {
      if (negtive & (j%2==0))
        matrix_filter[i][j] = -(j + N * i);
      else
        matrix_filter[i][j] = j + N * i;
      printf("%ld\t", matrix_filter[i][j]);
    }
    printf("\n");
  }
}

void matrix(int M, int K, int N, int negtive) {
  prepare_matrixAB(M, K, N, negtive);

  matrix_multiply2D_acc(M, N, K);

  printf("\nMatrix C =\n");
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      printf("%ld\t", matrix_result[i][j]);
    }
    printf("\n");
  }
}

void block_matrix(int M, int K, int N, int negtive) {

  prepare_matrixAB(M,K,N,negtive);
//----------------------------------------------------------------
  int fmaps_size = K;
  int fmaps_num = M;
  int filter_num = N;

  int blockSize = 16;
  int KK = int((fmaps_size + (blockSize-1))/blockSize)*blockSize;
  int MM = int((fmaps_num + (blockSize-1))/blockSize)*blockSize;
  int NN = int((filter_num + (blockSize-1))/blockSize)*blockSize;
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

  for (int i = 0; i < MM; i++) {
    for (int j = 0; j < NN; j++) {
      matrix_result[i][j] = 0;
    }
  }
//-------------------------------------------
  for (int ii = 0; ii < MM; ii+=blockSize)
    for (int jj = 0; jj < NN; jj+=blockSize)
      for (int kk = 0; kk < KK; kk+=blockSize)
	      matrix_multiply2D_acc_block(ii, jj, kk, blockSize);

  printf("\nMatrix C =\n");
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      printf("%ld\t", matrix_result[i][j]);
    }
    printf("\n");
  }
}

void calculate_2by3() { matrix(2, 3, 2, 0); }
void calculate_4by4() { matrix(4, 4, 4, 0); }
void calculate_5by5() { matrix(5, 5, 5, 0); }
void calculate_7by5() { matrix(7, 6, 5, 0); }

void block_matrix_7by5() { block_matrix(7, 6, 5, 0); }
void block_matrix_2by3() { block_matrix(2, 3, 3, 0); }
void block_matrix_4by4() { block_matrix(4, 4, 4, 0); }
void block_matrix_16by5() { block_matrix(16, 2, 5, 0); }
void block_matrix_17by5() { block_matrix(17, 2, 5, 0); }
void block_matrix_4by4n() { block_matrix(4, 4, 4, 1); }

struct Menu MENU = {
    "Project Menu",
    "project",
    {
        MENU_ITEM('0', "exercise cfu op0", do_exercise_cfu_op0),
        MENU_ITEM('g', "grid cfu op0", do_grid_cfu_op0),
        MENU_ITEM('h', "say Hello", do_hello_world),
        MENU_ITEM('r', "read_write_cfu", read_write_cfu),
        MENU_ITEM('2', "check 2*3", calculate_2by3),
        MENU_ITEM('4', "check 4*4", calculate_4by4),
        MENU_ITEM('5', "check 5*5", calculate_5by5),
        MENU_ITEM('7', "check 7*5", calculate_7by5),
        MENU_ITEM('b', "check block design of 7*5", block_matrix_7by5),
	      MENU_ITEM('s', "check block design of 2*3", block_matrix_2by3),
        MENU_ITEM('f', "check block design of 4*4", block_matrix_4by4),
        MENU_ITEM('l', "check block design of 16*5", block_matrix_16by5),
        MENU_ITEM('p', "check block design of 17*5", block_matrix_17by5),
        MENU_ITEM('n', "check block design of 4*4 with negtive", block_matrix_4by4n),
        MENU_END,
    },
};

};  // anonymous namespace

extern "C" void do_proj_menu() { menu_run(&MENU); }
