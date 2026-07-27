`timescale 1ns/1ps

module mux2_32(out,in1,in2,sel);

parameter v = 31 ;

input [v:0] in1,in2;
input sel;
output [v:0] out;

assign out = (sel==1'b0)?in1:in2;


endmodule
