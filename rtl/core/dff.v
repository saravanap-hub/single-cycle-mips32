`timescale 1ns / 1ps

module d_ff #(parameter WIDTH = 31)
(
    input                   clk,
    input                   reset,
    input      [WIDTH:0]  d,
    output reg [WIDTH:0]  q
);
    always @(posedge clk or posedge reset)
        if (reset) q <= 0;
        else       q <= d;
        
endmodule
