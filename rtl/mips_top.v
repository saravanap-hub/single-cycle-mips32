`timescale 1ns/1ps

module mips_top(
    input         clk, reset,
    output [31:0] writedata,
    output [31:0] dataadr,
    output        memwrite
);

    wire [31:0] pc, instr, readdata;
    
 
    mips core(
        .clk(clk), .reset(reset),
        .pc(pc), .instr(instr),
        .memwrite(memwrite),
        .aluout(dataadr),
        .writedata(writedata),
        .readdata(readdata)
    );
 
    imem imem_inst(pc[7:2], instr);
    dmem dmem_inst(clk, memwrite, dataadr, writedata, readdata);

endmodule