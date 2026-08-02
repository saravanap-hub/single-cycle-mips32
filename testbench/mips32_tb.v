`timescale 1ns/1ps
 
module testbench();
    reg         clk;
    reg         reset;
    wire [31:0] writedata, dataadr;
    wire        memwrite;
 
    mips_top dut(
        .clk(clk), .reset(reset),
        .writedata(writedata),
        .dataadr(dataadr),
        .memwrite(memwrite)
    );
 
    
    initial begin
        reset <= 1; # 22; reset <= 0;
    end
 
   
    always begin
        clk <= 1; # 5; clk <= 0; # 5;
    end
 
    
    always @(negedge clk) begin
        if (memwrite) begin
            if (dataadr === 84 && writedata === 7) begin
                $display("Simulation succeeded");
                $stop;
            end 
            else if (dataadr !== 80) begin
                $display("Simulation failed");
                $stop;
            end
        end
    end
endmodule