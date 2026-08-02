`timescale 1ns/1ps

module datapath(
    input         clk, reset,
    input         memtoreg, pcsrc,
    input         alusrc, regdst,
    input         regwrite, jump,
    input  [2:0]  alucontrol,
    output        zero,
    output [31:0] pc,
    input  [31:0] instr,
    output [31:0] aluout,
    output [31:0] writedata,
    input  [31:0] readdata
);
    wire [4:0]  writereg;
    wire [31:0] pcnext, pcnextbr, pcplus4, pcbranch;
    wire [31:0] signimm, signimmsh;
    wire [31:0] srca, result;
    wire [31:0] srcb;
    wire [27:0] jumpshift;
 
    
    dff #(32) pcreg(clk, reset, pcnext, pc);
    adder       pcadd1(pc, 32'd4, pcplus4);
    signext     se(instr[15:0], signimm);
    assign      signimmsh = signimm << 2;             
    adder       pcadd2(pcplus4, signimmsh, pcbranch);
    mux2 #(32)  pcbrmux(pcplus4, pcbranch, pcsrc, pcnextbr);
    assign      jumpshift = {instr[25:0], 2'b00};       
    mux2 #(32)  pcmux(pcnextbr, {pcplus4[31:28], jumpshift}, jump, pcnext);
 

    regfile     rf(clk, regwrite, instr[25:21], instr[20:16], writereg, result, srca, writedata);
    mux2 #(5)   wrmux(instr[20:16], instr[15:11], regdst, writereg);
    mux2 #(32)  resmux(aluout, readdata, memtoreg, result);
 
    
    mux2 #(32)  srcbmux(writedata, signimm, alusrc, srcb);
    alu         alu_inst(srca, srcb, alucontrol, aluout, zero);
endmodule