`timescale 1ns/1ps

module regfile(
    input         clk,
    input         we3,          // write enable (from control unit: regwrite)
    input  [4:0]  ra1, ra2,     // read addresses  (rs, rt)
    input  [4:0]  wa3,          // write address   (rd or rt, chosen by regdst mux)
    input  [31:0] wd3,          // write data      (ALU result or memory data)
    output [31:0] rd1, rd2      // read data outputs (srcA, writedata)
);
    reg [31:0] rf [31:0];
 


    // Synchronous write
    always @(posedge clk)
        if (we3) rf[wa3] <= wd3;

        
 
    // Asynchronous (combinational) read, with $0 hardwired to 0
    assign rd1 = (ra1 != 5'b0) ? rf[ra1] : 32'b0;
    assign rd2 = (ra2 != 5'b0) ? rf[ra2] : 32'b0;

endmodule