// Copyright 2021 The CFU-Playground Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//`include "RTL/PATTERN.v"
`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/global_buffer.v"
`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/TPU.v"
`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/block.v"
`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/systolic_array.v"
`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/TPU_fsm.v"




module Cfu
#(  parameter ADDR_BITS=12
) (
  input               cmd_valid,
  output              cmd_ready,
  input      [9:0]    cmd_payload_function_id,
  input      [31:0]   cmd_payload_inputs_0,
  input      [31:0]   cmd_payload_inputs_1,
  output reg          rsp_valid,
  input               rsp_ready,
  output reg  [31:0]  rsp_payload_outputs_0,
  input               reset,
  input               clk
);

/*
	PATTERN My_Pattern(
// CFU I/O
	    .cmd_valid			(cmd_valid),
	    .cmd_ready			(cmd_ready),
	    .cmd_payload_function_id	(cmd_payload_function_id),
	    .cmd_payload_inputs_0	(cmd_payload_inputs_0),
	    .cmd_payload_inputs_1	(cmd_payload_inputs_1),
	    .rsp_valid			(rsp_valid),
	    .rsp_ready			(rsp_ready),
	    .rsp_payload_outputs_0	(rsp_payload_outputs_0),
	    .reset			(reset),

	    .clk            (clk),     
	    .rst_n          (rst_n),     
	    .in_valid       (in_valid),         
	    .K              (K), 
	    .M              (M), 
	    .N              (N), 
	    .busy           (busy),     
	    .A_wr_en        (A_wr_en),         
	    .A_index        (A_index),         
	    .A_data_in      (A_data_in),         
	    .A_data_out     (A_data_out),         
	    .B_wr_en        (B_wr_en),         
	    .B_index        (B_index),         
	    .B_data_in      (B_data_in),         
	    .B_data_out     (B_data_out),         
	    .C_wr_en        (C_wr_en),         
	    .C_index        (C_index),         
	    .C_data_in      (C_data_in),         
	    .C_data_out     (C_data_out)         
	);
*/
/*
	TPU My_TPU(
	    .clk            (clk),     
	    .rst_n          (rst_n),     
	    .in_valid       (in_valid),         
	    .K              (K), 
	    .M              (M), 
	    .N              (N), 
	    .busy           (busy),     
	    .A_wr_en        (A_wr_en),         
	    .A_index        (A_index),         
	    .A_data_in      (A_data_in),         
	    .A_data_out     (A_data_out),         
	    .B_wr_en        (B_wr_en),         
	    .B_index        (B_index),         
	    .B_data_in      (B_data_in),         
	    .B_data_out     (B_data_out),         
	    .C_wr_en        (C_wr_en),         
	    .C_index        (C_index),         
	    .C_data_in      (C_data_in),         
	    .C_data_out     (C_data_out)         
	);
*/
/*
  // Trivial handshaking for a combinational CFU
  assign rsp_valid = cmd_valid;
  assign cmd_ready = rsp_ready;

  //
  // select output -- note that we're not fully decoding the 3 function_id bits
  //
  assign rsp_payload_outputs_0 = cmd_payload_function_id[0] ? 
                                           cmd_payload_inputs_1 :
                                           cmd_payload_inputs_0 ;
*/

	global_buffer #(
	    .ADDR_BITS(ADDR_BITS),
	    .DATA_BITS(32)
	)
	gbuff_A(
	    .clk(clk),
	    .rst_n(rst_n),
	    .wr_en(A_wr_en_mux),
	    .index(A_index_mux),
	    .data_in(A_data_in_mux),
	    .data_out(A_data_out)
	);

	global_buffer #(
	    .ADDR_BITS(ADDR_BITS),
	    .DATA_BITS(32)
	) gbuff_B(
	    .clk(clk),
	    .rst_n(rst_n),
	    .wr_en(B_wr_en_mux),
	    .index(B_index_mux),
	    .data_in(B_data_in_mux),
	    .data_out(B_data_out)
	);


	global_buffer #(
	    .ADDR_BITS(ADDR_BITS),
	    .DATA_BITS(128)
	) gbuff_C(
	    .clk(clk),
	    .rst_n(rst_n),
	    .wr_en(C_wr_en),
	    .index(C_index),
	    .data_in(C_data_in),
	    .data_out(C_data_out)
	);

//---------------------------------------------------------------------------------------
reg rst_n;
reg in_valid;
reg [31:0] input0_reg;
reg [31:0] input1_reg;
//assign rst_n = ~reset;
//assign in_valid = cmd_valid;

reg [31:0] K, M, N;
wire [6:0] op;
assign op = cmd_payload_function_id[9:3];

wire A_wr_en_mux;
wire B_wr_en_mux;
wire [ADDR_BITS-1:0] A_index_mux;
wire [ADDR_BITS-1:0] B_index_mux;
wire [31:0] A_data_in_mux;
wire [31:0] B_data_in_mux;

reg A_wr_en_init;
reg B_wr_en_init;
reg [ADDR_BITS-1:0] A_index_init;
reg [ADDR_BITS-1:0] B_index_init;
reg [31:0] A_data_in_init;
reg [31:0] B_data_in_init;

/*
assign A_wr_en_mux = (in_valid) ? A_wr_en : A_wr_en_init;
assign B_wr_en_mux = (in_valid) ? B_wr_en : B_wr_en_init;
assign A_index_mux = (in_valid) ? A_index : A_index_init;
assign B_index_mux = (in_valid) ? B_index : B_index_init;
assign A_data_in_mux = (in_valid) ? A_data_in : A_data_in_init;
assign B_data_in_mux = (in_valid) ? B_data_in : B_data_in_init;
*/
assign A_wr_en_mux =  A_wr_en_init;
assign B_wr_en_mux =  B_wr_en_init;
assign A_index_mux =  A_index_init;
assign B_index_mux =  B_index_init;
assign A_data_in_mux =  A_data_in_init;
assign B_data_in_mux =  B_data_in_init;
  // Only not ready for a command when we have a response.
  assign cmd_ready = ~rsp_valid;
  always @(posedge clk) begin

//    input0_reg <= cmd_payload_inputs_0;
//    input1_reg <= cmd_payload_inputs_1;
    if (reset) begin
      rsp_payload_outputs_0 <= 32'b0;
      rsp_valid <= 1'b0;
      in_valid <= 1'b0;
    end else if (rsp_valid) begin
      // Waiting to hand off response to CPU.
      rsp_valid <= ~rsp_ready;
    end else if (cmd_valid) begin
      rsp_valid <= 1'b1;
      // Accumulate step:
      case (op)
	7'd1: begin // Reset
	  rst_n <= 1'b0;
    	  in_valid <= 1'b0;
    	  K = 'bx;
    	  M = 'bx;
    	  N = 'bx;
	  
	end
        7'd2: begin // Set parameter K
	  rst_n <= 1'b1;
	  K <= cmd_payload_inputs_0;

	end
        7'd3: begin // Read parameter K
	  rst_n <= 1'b1;
	  rsp_payload_outputs_0 <= K;

	end

        7'd4: begin // Set parameter M
	  rst_n <= 1'b1;
	  M <= cmd_payload_inputs_0;

	end
        7'd5: begin // Read parameter M
	  rst_n <= 1'b1;
	  rsp_payload_outputs_0<= M;

	end

        7'd6: begin // Set parameter N
	  rst_n <= 1'b1;
	  N <= cmd_payload_inputs_0;

	end
        7'd7: begin // Read parameter N
	  rst_n <= 1'b1;
	  rsp_payload_outputs_0 <= N;

	end

        7'd8: begin // Set global bufer A
	  rst_n <= 1'b1;
	  A_index_init <= cmd_payload_inputs_0[ADDR_BITS-1:0];
	  A_data_in_init <= cmd_payload_inputs_1;
	  A_wr_en_init <= 1'b1;
	  rsp_payload_outputs_0 <= cmd_payload_inputs_0[ADDR_BITS-1:0]; //check index

	end

        7'd9: begin // Read global bufer A
	  rst_n <= 1'b1;
	  A_wr_en_init <= 1'b0;
	  A_index_init <= cmd_payload_inputs_0[9:0];
	  if (A_wr_en_init == 1'b0) begin
	    rsp_payload_outputs_0 <= A_data_out;

	  end
	end

        7'd10: begin // Set global bufer B
	  rst_n <= 1'b1;
	  B_index_init <= cmd_payload_inputs_0;
	  B_data_in_init <= cmd_payload_inputs_1;
	  B_wr_en_init <= 1'b1;
	  //gbuff_B.gbuff[cmd_payload_inputs_0] = cmd_payload_inputs_1;
	end
        7'd11: begin // Read global bufer A
	  rst_n <= 1'b1;
	  B_wr_en_init <= 1'b0;
	  B_index_init <= cmd_payload_inputs_0;
	  rsp_payload_outputs_0 <= B_data_out;
	  //rsp_payload_outputs_0 <= gbuff_B.gbuff[cmd_payload_inputs_0];
	end
	default begin
	  rst_n <= 1'b1;
	  A_wr_en_init <= 1'b0;
	  B_wr_en_init <= 1'b0;
	end
/*
        7'd12: begin // Start Calculate
	  rst_n <= 1'b1;
	  in_valid <= 1;
	end
        7'd13: begin //Check status
	  rsp_payload_outputs_0 <= busy;
	end
	7'd14: begin // Read global buffer C
	  rsp_payload_outputs_0 <= gbuff_C.gbuff[cmd_payload_inputs_0];
	end
*/
	endcase
    end
  end
endmodule
