//`include "systolic_array.v"
//`include "TPU_fsm.v"
module TPU #(
    parameter ADDR_BITS = 16,
    parameter DATA_BITS = 8,
    parameter DATA_BITS_BLOCK_IN = DATA_BITS * 2,
    parameter DATA_BITS_BLOCK_OUT = DATA_BITS_BLOCK_IN * 2,
    parameter DATA_BITS_LB_IN = (DATA_BITS * 2) * 4,
    parameter DATA_BITS_LB_OUT = DATA_BITS_LB_IN * 2,
    parameter DATA_BITS_GB_IN = DATA_BITS * 4,
    parameter DATA_BITS_GB_OUT = ((DATA_BITS * 2) * 4) * 2
) (
    input clk,
    input rst_n,
    input in_valid,
    input [31:0] K,
    input [31:0] M,
    input [31:0] N,
    output busy,
    output [3:0] state_TPU_o,
    output [2:0] state_SA_o,

    output [DATA_BITS_LB_IN-1:0] local_buffer_A0_o,
    output [DATA_BITS_LB_IN-1:0] local_buffer_A1_o,
    output [DATA_BITS_LB_IN-1:0] local_buffer_A2_o,
    output [DATA_BITS_LB_IN-1:0] local_buffer_A3_o,

    output [DATA_BITS_LB_IN-1:0] local_buffer_B0_o,
    output [DATA_BITS_LB_IN-1:0] local_buffer_B1_o,
    output [DATA_BITS_LB_IN-1:0] local_buffer_B2_o,
    output [DATA_BITS_LB_IN-1:0] local_buffer_B3_o,

    output [DATA_BITS_LB_OUT-1:0] local_buffer_C0_o,
    output [DATA_BITS_LB_OUT-1:0] local_buffer_C1_o,
    output [DATA_BITS_LB_OUT-1:0] local_buffer_C2_o,
    output [DATA_BITS_LB_OUT-1:0] local_buffer_C3_o,

    output [DATA_BITS_BLOCK_OUT-1:0] result0_o,

    output [DATA_BITS_BLOCK_IN-1:0] inp_north0_o,
    output [DATA_BITS_BLOCK_IN-1:0] inp_north1_o,
    output [DATA_BITS_BLOCK_IN-1:0] inp_north2_o,
    output [DATA_BITS_BLOCK_IN-1:0] inp_north3_o,
    output [DATA_BITS_BLOCK_IN-1:0] inp_west0_o,
    output [DATA_BITS_BLOCK_IN-1:0] inp_west4_o,
    output [DATA_BITS_BLOCK_IN-1:0] inp_west8_o,
    output [DATA_BITS_BLOCK_IN-1:0] inp_west12_o,

    output A_wr_en,
    output [ADDR_BITS-1:0] A_index,
    output [DATA_BITS_GB_IN-1:0] A_data_in,
    input [DATA_BITS_GB_IN-1:0] A_data_out,

    output B_wr_en,
    output [ADDR_BITS-1:0] B_index,
    output [DATA_BITS_GB_IN-1:0] B_data_in,
    input [DATA_BITS_GB_IN:0] B_data_out,

    output C_wr_en,
    output [ADDR_BITS-1:0] C_index,
    output [DATA_BITS_GB_OUT:0] C_data_in,
    input [DATA_BITS_GB_OUT:0] C_data_out
);

  assign local_buffer_A0_o = local_buffer_A0;
  assign local_buffer_A1_o = local_buffer_A1;
  assign local_buffer_A2_o = local_buffer_A2;
  assign local_buffer_A3_o = local_buffer_A3;

  assign local_buffer_B0_o = local_buffer_B0;
  assign local_buffer_B1_o = local_buffer_B1;
  assign local_buffer_B2_o = local_buffer_B2;
  assign local_buffer_B3_o = local_buffer_B3;

  assign local_buffer_C0_o = local_buffer_C0;
  assign local_buffer_C1_o = local_buffer_C1;
  assign local_buffer_C2_o = local_buffer_C2;
  assign local_buffer_C3_o = local_buffer_C3;

  wire sa_rst_n;
  wire [DATA_BITS_LB_IN-1:0] local_buffer_A0;
  wire [DATA_BITS_LB_IN-1:0] local_buffer_A1;
  wire [DATA_BITS_LB_IN-1:0] local_buffer_A2;
  wire [DATA_BITS_LB_IN-1:0] local_buffer_A3;
  wire [DATA_BITS_LB_IN-1:0] local_buffer_B0;
  wire [DATA_BITS_LB_IN-1:0] local_buffer_B1;
  wire [DATA_BITS_LB_IN-1:0] local_buffer_B2;
  wire [DATA_BITS_LB_IN-1:0] local_buffer_B3;
  wire [DATA_BITS_LB_OUT-1:0] local_buffer_C0;
  wire [DATA_BITS_LB_OUT-1:0] local_buffer_C1;
  wire [DATA_BITS_LB_OUT-1:0] local_buffer_C2;
  wire [DATA_BITS_LB_OUT-1:0] local_buffer_C3;

  TPU_fsm TPU_fsm1 (
      .clk(clk),
      .rst_n(rst_n),
      .state_TPU_o(state_TPU_o),
      .in_valid(in_valid),
      .done(done),
      .K(K),
      .M(M),
      .N(N),

      .busy(busy),
      .sa_rst_n(sa_rst_n),
      // Global Buffer A control
      .A_wr_en(A_wr_en),
      .A_index(A_index),
      .A_data_out(A_data_out),
      // Global Buffer B control
      .B_wr_en(B_wr_en),
      .B_index(B_index),
      .B_data_out(B_data_out),
      // Global Buffer C control
      .C_wr_en(C_wr_en),
      .C_index(C_index),
      .C_data_in(C_data_in),
      // Local Buffer A control
      .local_buffer_A0(local_buffer_A0),
      .local_buffer_A1(local_buffer_A1),
      .local_buffer_A2(local_buffer_A2),
      .local_buffer_A3(local_buffer_A3),
      // Local Buffer B control
      .local_buffer_B0(local_buffer_B0),
      .local_buffer_B1(local_buffer_B1),
      .local_buffer_B2(local_buffer_B2),
      .local_buffer_B3(local_buffer_B3),
      // Local Buffer C control
      .local_buffer_C0(local_buffer_C0),
      .local_buffer_C1(local_buffer_C1),
      .local_buffer_C2(local_buffer_C2),
      .local_buffer_C3(local_buffer_C3)
  );

  systolic_array #(
      .DATA_BITS(DATA_BITS * 2)
  ) systolic_array1 (

      .clk(clk),
      .sa_rst_n(sa_rst_n),
      .state_SA_o(state_SA_o),
      .busy(busy),
      .done(done),

      // debug
      .inp_north0_o(inp_north0_o),
      .inp_north1_o(inp_north1_o),
      .inp_north2_o(inp_north2_o),
      .inp_north3_o(inp_north3_o),
      .inp_west0_o (inp_west0_o),
      .inp_west4_o (inp_west4_o),
      .inp_west8_o (inp_west8_o),
      .inp_west12_o(inp_west12_o),

      .result0_o(result0_o),
      .local_buffer_A0(local_buffer_A0),
      .local_buffer_A1(local_buffer_A1),
      .local_buffer_A2(local_buffer_A2),
      .local_buffer_A3(local_buffer_A3),

      .local_buffer_B0(local_buffer_B0),
      .local_buffer_B1(local_buffer_B1),
      .local_buffer_B2(local_buffer_B2),
      .local_buffer_B3(local_buffer_B3),

      .local_buffer_C0(local_buffer_C0),
      .local_buffer_C1(local_buffer_C1),
      .local_buffer_C2(local_buffer_C2),
      .local_buffer_C3(local_buffer_C3)
  );

endmodule
