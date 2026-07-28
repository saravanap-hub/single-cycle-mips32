`timescale 1ps/1ps

module alu(
    input      [31:0] a, b,
    input      [2:0]  alucontrol,
    output reg [31:0] result,
    output             zero
);
    wire [31:0] b2;
    wire [31:0] sum;
 
    assign b2  = alucontrol[2] ? ~b : b;
    assign sum = a + b2 + alucontrol[2];
 
    always @(*)
        case (alucontrol[1:0])
            2'b00: result = a & b2;      // AND
            2'b01: result = a | b2;      // OR
            2'b10: result = sum;         // ADD or SUB
            2'b11: result = sum[31];     // SLT (sign bit of a-b)
            default: result = 32'bx;
        endcase
 
    assign zero = (result == 32'b0);
endmodule