/*

MIT License

Copyright (c) 2020 Debtanu Mukherjee

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

module block #(
    parameter DATA_BITS = 16
) (
    input signed [DATA_BITS-1:0] inp_north,
    input signed [DATA_BITS-1:0] inp_west,
    input clk,
    input rst,
    output reg signed [DATA_BITS-1:0] outp_south,
    output reg signed [DATA_BITS-1:0] outp_east,
    output reg signed [DATA_BITS*2-1:0] result
);

  wire [DATA_BITS*2-1:0] multi;

  always @(posedge rst or posedge clk) begin
    if (rst) begin
      result <= 0;
      outp_east <= 0;
      outp_south <= 0;
    end else begin
      result <= result + multi;
      outp_east <= inp_west;
      outp_south <= inp_north;
    end
  end

  assign multi = inp_north * inp_west;

endmodule
