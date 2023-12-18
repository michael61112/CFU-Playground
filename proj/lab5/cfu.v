`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/global_buffer.v"
`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/TPU.v"
`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/block.v"
`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/systolic_array.v"
`include "/home/michael/hw/CFU-Playground/proj/lab5/RTL/TPU_fsm.v"

module Cfu #(
    parameter ADDR_BITS = 8,
    parameter DATA_BITS = 8,
    parameter DATA_BITS_GB_IN = DATA_BITS * 4,
    parameter DATA_BITS_GB_OUT = ((DATA_BITS * 2) * 4) * 2,
    parameter S0 = 4'b0000,
    parameter S1 = 4'b0001,
    parameter S2 = 4'b0010,
    parameter S3 = 4'b0011,
    parameter S4 = 4'b0100,
    parameter S5 = 4'b0101,
    parameter S6 = 4'b0110,
    parameter S7 = 4'b0111,
    parameter S8 = 4'b1000,
    parameter S9 = 4'b1001
) (
    input             cmd_valid,
    output reg        cmd_ready,
    input      [ 9:0] cmd_payload_function_id,
    input      [31:0] cmd_payload_inputs_0,
    input      [31:0] cmd_payload_inputs_1,
    output reg        rsp_valid,
    input             rsp_ready,
    output reg [31:0] rsp_payload_outputs_0,
    input             reset,
    input             clk
);

  global_buffer #(
      .ADDR_BITS(ADDR_BITS),
      .DATA_BITS(DATA_BITS_GB_IN)
  ) gbuff_A (
      .clk(clk),
      .rst_n(rst_n),
      .wr_en(A_wr_en_mux),
      .index(A_index_mux),
      .data_in(A_data_in_mux),
      .data_out(A_data_out)
  );

  global_buffer #(
      .ADDR_BITS(ADDR_BITS),
      .DATA_BITS(DATA_BITS_GB_IN)
  ) gbuff_B (
      .clk(clk),
      .rst_n(rst_n),
      .wr_en(B_wr_en_mux),
      .index(B_index_mux),
      .data_in(B_data_in_mux),
      .data_out(B_data_out)
  );

  global_buffer #(
      .ADDR_BITS(ADDR_BITS),
      .DATA_BITS(DATA_BITS_GB_OUT)
  ) gbuff_C (
      .clk(clk),
      .rst_n(rst_n),
      .wr_en(C_wr_en_mux),
      .index(C_index_mux),
      .data_in(C_data_in_mux),
      .data_out(C_data_out)
  );

  TPU My_TPU (
      .clk        (clk),
      .rst_n      (rst_n),
      .state_TPU_o(state_TPU_o),
      .in_valid   (in_valid),
      .K          (K),
      .M          (M),
      .N          (N),
      .busy       (busy),
      .A_wr_en    (A_wr_en),
      .A_index    (A_index),
      .A_data_in  (A_data_in),
      .A_data_out (A_data_out),
      .B_wr_en    (B_wr_en),
      .B_index    (B_index),
      .B_data_in  (B_data_in),
      .B_data_out (B_data_out),
      .C_wr_en    (C_wr_en),
      .C_index    (C_index),
      .C_data_in  (C_data_in),
      .C_data_out (C_data_out),
      .inputOffset(inputOffset)
  );

  reg rst_n;
  reg in_valid;

  reg [31:0] K, M, N;
  wire [DATA_BITS_GB_IN-1:0] A_data_out, B_data_out;
  wire [DATA_BITS_GB_OUT-1:0] C_data_out;
  wire [6:0] op;
  wire busy;
  assign op = cmd_payload_function_id[9:3];

  wire A_wr_en_mux;
  wire B_wr_en_mux;
  wire C_wr_en_mux;
  wire [ADDR_BITS-1:0] A_index_mux;
  wire [ADDR_BITS-1:0] B_index_mux;
  wire [ADDR_BITS-1:0] C_index_mux;
  wire [DATA_BITS_GB_IN-1:0] A_data_in_mux;
  wire [DATA_BITS_GB_IN-1:0] B_data_in_mux;
  wire [DATA_BITS_GB_OUT-1:0] C_data_in_mux;

  reg A_wr_en_init;
  reg B_wr_en_init;
  reg C_wr_en_init;
  reg [DATA_BITS-1:0] inputOffset;

  wire A_wr_en, B_wr_en, C_wr_en;
  wire [ADDR_BITS-1:0] A_index, B_index, C_index;
  wire [DATA_BITS_GB_IN-1:0] A_data_in, B_data_in;
  wire [DATA_BITS_GB_OUT-1:0] C_data_in;
  wire [3:0] state_TPU_o;
  reg [ADDR_BITS-1:0] A_index_init;
  reg [ADDR_BITS-1:0] B_index_init;
  reg [ADDR_BITS-1:0] C_index_init;

  reg [DATA_BITS_GB_IN-1:0] A_data_in_init;
  reg [DATA_BITS_GB_IN-1:0] B_data_in_init;
  reg [DATA_BITS_GB_OUT-1:0] C_data_in_init;

  assign A_wr_en_mux   = (in_valid | busy) ? A_wr_en : A_wr_en_init;
  assign B_wr_en_mux   = (in_valid | busy) ? B_wr_en : B_wr_en_init;
  assign C_wr_en_mux   = (busy) ? C_wr_en : C_wr_en_init;

  assign A_index_mux   = (in_valid | busy) ? A_index : A_index_init;
  assign B_index_mux   = (in_valid | busy) ? B_index : B_index_init;
  assign C_index_mux   = (busy) ? C_index : C_index_init;

  assign A_data_in_mux = (in_valid) ? A_data_in : A_data_in_init;
  assign B_data_in_mux = (in_valid) ? B_data_in : B_data_in_init;
  assign C_data_in_mux = (busy) ? C_data_in : C_data_in_init;

  reg [3:0] state;
  always @(negedge clk) begin
    if (reset) begin
      state <= S0;
    end else begin
      case (state)
        S0: begin
          if (cmd_valid) begin
            state <= S1;
          end else begin
            state <= S0;
          end
        end
        S1: begin
          if (op == 9) begin  // Read Buffer A
            state <= S2;
          end else if (op == 11) begin  //Read Buffer B
            state <= S5;
          end else if (op == 14) begin  //Read Buffer C0
            state <= S6;
          end else if (op == 15) begin  //Read Buffer C1
            state <= S7;
          end else if (op == 16) begin  //Read Buffer C2
            state <= S8;
          end else if (op == 17) begin  //Read Buffer C3
            state <= S9;
          end else begin
            state <= S3;
          end
        end
        S2: begin
          state <= S3;
        end
        S3: begin
          if (rsp_ready) begin
            state <= S4;
          end else begin
            state <= S3;
          end
        end
        S4: begin
          state <= S0;
        end
        S5: begin
          state <= S3;
        end
        S6: begin
          state <= S3;
        end
        S7: begin
          state <= S3;
        end
        S8: begin
          state <= S3;
        end
        S9: begin
          state <= S3;
        end
      endcase
    end
  end

  // Set output value
  always @(posedge clk) begin
    case (state)
      S0: begin
        cmd_ready <= 1'b0;
        rsp_valid <= 1'b0;
        rst_n <= 1'b1;
        in_valid <= 1'b0;
      end
      S1: begin
        cmd_ready <= 1'b1;
        case (op)
          7'd1: begin  // Reset
            rst_n <= cmd_payload_inputs_0[0];
            K = 'bx;
            M = 'bx;
            N = 'bx;
            inputOffset = 'bx;
          end
          7'd2: begin  // Set parameter K
            K <= cmd_payload_inputs_0;
          end

          7'd3: begin  // Read parameter K
            rsp_payload_outputs_0 <= K;
          end

          7'd4: begin  // Set parameter M
            M <= cmd_payload_inputs_0;
          end

          7'd5: begin  // Read parameter M
            rsp_payload_outputs_0 <= M;
          end

          7'd6: begin  // Set parameter N
            N <= cmd_payload_inputs_0;
          end

          7'd7: begin  // Read parameter N
            rsp_payload_outputs_0 <= N;
          end

          7'd8: begin  // Set global bufer A
            A_index_init   <= cmd_payload_inputs_0[ADDR_BITS-1:0];
            A_data_in_init <= cmd_payload_inputs_1;
            A_wr_en_init   <= 1'b1;
          end
          7'd9: begin  // Read global bufer A
            A_wr_en_init <= 1'b0;
            A_index_init <= cmd_payload_inputs_0[ADDR_BITS-1:0];
          end
          7'd10: begin  // Set global bufer B
            A_wr_en_init   <= 1'b0;
            B_index_init   <= cmd_payload_inputs_0[ADDR_BITS-1:0];
            B_data_in_init <= cmd_payload_inputs_1;
            B_wr_en_init   <= 1'b1;
          end
          7'd11: begin  // Read global bufer B
            A_wr_en_init <= 1'b0;
            B_wr_en_init <= 1'b0;
            B_index_init <= cmd_payload_inputs_0[ADDR_BITS-1:0];
          end
          7'd12: begin  // Set in_valid
            A_wr_en_init <= 1'b0;
            B_wr_en_init <= 1'b0;
            in_valid <= 1'b1;
          end
          7'd13: begin  // Read busy
            rsp_payload_outputs_0 <= busy;
          end
          7'd14: begin  // Read global bufer C
            C_wr_en_init <= 1'b0;
            C_index_init <= cmd_payload_inputs_0[ADDR_BITS-1:0];
          end
          7'd15: begin  // Read global bufer C
            C_wr_en_init <= 1'b0;
            C_index_init <= cmd_payload_inputs_0[ADDR_BITS-1:0];
          end
          7'd16: begin  // Read global bufer C
            C_wr_en_init <= 1'b0;
            C_index_init <= cmd_payload_inputs_0[ADDR_BITS-1:0];
          end
          7'd17: begin  // Read global bufer C
            C_wr_en_init <= 1'b0;
            C_index_init <= cmd_payload_inputs_0[ADDR_BITS-1:0];
          end
          7'd18: begin  // Set InputOffset
            inputOffset <= cmd_payload_inputs_0[DATA_BITS-1:0];
          end
        endcase
      end
      S2: begin  // Wait one cycle output buffer A
        cmd_ready <= 1'b0;
        rsp_valid <= 1'b0;
        rsp_payload_outputs_0 <= A_data_out;
      end
      S3: begin
        cmd_ready <= 1'b0;
        rsp_valid <= 1'b1;
      end
      S4: begin
        cmd_ready <= 1'b0;
        rsp_valid <= 1'b0;
      end
      S5: begin  // Wait one cycle output buffer B
        cmd_ready <= 1'b0;
        rsp_valid <= 1'b0;
        rsp_payload_outputs_0 <= B_data_out;
      end
      S6: begin  // Wait one cycle output buffer C
        cmd_ready <= 1'b0;
        rsp_valid <= 1'b0;
        rsp_payload_outputs_0 <= C_data_out[DATA_BITS_GB_IN-1:0];
      end
      S7: begin  // Wait one cycle output buffer C
        cmd_ready <= 1'b0;
        rsp_valid <= 1'b0;
        rsp_payload_outputs_0 <= C_data_out[DATA_BITS_GB_IN*2-1:DATA_BITS_GB_IN];
      end
      S8: begin  // Wait one cycle output buffer C
        cmd_ready <= 1'b0;
        rsp_valid <= 1'b0;
        rsp_payload_outputs_0 <= C_data_out[DATA_BITS_GB_IN*3-1:DATA_BITS_GB_IN*2];
      end
      S9: begin  // Wait one cycle output buffer C
        cmd_ready <= 1'b0;
        rsp_valid <= 1'b0;
        rsp_payload_outputs_0 <= C_data_out[DATA_BITS_GB_OUT-1:DATA_BITS_GB_IN*3];
      end
    endcase
  end

endmodule
