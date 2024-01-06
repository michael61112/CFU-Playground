module Cfu (
  input               cmd_valid,
  output              cmd_ready,
  input      [9:0]    cmd_payload_function_id,
  input      [31:0]   cmd_payload_inputs_0,
  input      [31:0]   cmd_payload_inputs_1,
  output reg          rsp_valid,
  input               rsp_ready,
  output reg [31:0]   rsp_payload_outputs_0,
  input               reset,
  input               clk
);
  reg [15:0] InputOffset;

  // SIMD multiply step:
  wire signed [16:0] prod_0, prod_1, prod_2, prod_3, prod_4, prod_5, prod_6, prod_7;
  reg [7:0] a0, a1, a2, a3, a4, a5, a6, a7;
  reg [7:0] b0, b1, b2, b3, b4, b5, b6, b7;

  assign prod_0 =  ($signed(a0) + $signed(InputOffset))
                  * $signed(b0);
  assign prod_1 =  ($signed(a1) + $signed(InputOffset))
                  * $signed(b1);
  assign prod_2 =  ($signed(a2) + $signed(InputOffset))
                  * $signed(b2);
  assign prod_3 =  ($signed(a3) + $signed(InputOffset))
                  * $signed(b3);

  assign prod_4 =  ($signed(a4) + $signed(InputOffset))
                  * $signed(b4);
  assign prod_5 =  ($signed(a5) + $signed(InputOffset))
                  * $signed(b5);
  assign prod_6 =  ($signed(a6) + $signed(InputOffset))
                  * $signed(b6);
  assign prod_7 =  ($signed(a7) + $signed(InputOffset))
                  * $signed(b7);

  wire signed [31:0] sum_prods;
  assign sum_prods = prod_0 + prod_1 + prod_2 + prod_3; // + prod_4 + prod_5 + prod_6 + prod_7;

  // Only not ready for a command when we have a response.
  assign cmd_ready = ~rsp_valid;

  always @(posedge clk) begin
    if (reset) begin
      rsp_payload_outputs_0 <= 32'b0;
      rsp_valid <= 1'b0;
      InputOffset <= 0;
    end else if (rsp_valid) begin
      // Waiting to hand off response to CPU.
      rsp_valid <= ~rsp_ready;
    end else if (cmd_valid) begin
      rsp_valid <= 1'b1;
      // Accumulate step:
      case (cmd_payload_function_id[9:3])
        2'b000_0000: begin
          InputOffset <= InputOffset;
          rsp_payload_outputs_0 <= rsp_payload_outputs_0 + sum_prods;
        end
        2'b000_0001: begin
          InputOffset <= cmd_payload_inputs_0[15:0];
          rsp_payload_outputs_0 <= 0'b0;
        end
        7'd2: begin
          InputOffset <= InputOffset;
          a0 <= cmd_payload_inputs_0[7 : 0];
          a1 <= cmd_payload_inputs_0[15: 8];
          a2 <= cmd_payload_inputs_0[23:16];
          a3 <= cmd_payload_inputs_0[31:24];
          b0 <= cmd_payload_inputs_1[7 : 0];
          b1 <= cmd_payload_inputs_1[15: 8];
          b2 <= cmd_payload_inputs_1[23:16];
          b3 <= cmd_payload_inputs_1[31:24];
        end
        7'd3: begin
          InputOffset <= InputOffset;
          a4 <= cmd_payload_inputs_0[7 : 0];
          a5 <= cmd_payload_inputs_0[15: 8];
          a6 <= cmd_payload_inputs_0[23:16];
          a7 <= cmd_payload_inputs_0[31:24];
          b4 <= cmd_payload_inputs_1[7 : 0];
          b5 <= cmd_payload_inputs_1[15: 8];
          b6 <= cmd_payload_inputs_1[23:16];
          b7 <= cmd_payload_inputs_1[31:24];
        end
        7'd4: begin
          InputOffset <= InputOffset;
          rsp_payload_outputs_0 <= rsp_payload_outputs_0 + sum_prods;
        end

        default: begin
          InputOffset <= InputOffset;
          rsp_payload_outputs_0 <= 0'b0;
        end
      endcase
    end
  end
endmodule