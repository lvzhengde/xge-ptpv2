/*++
//  testcase for ptpv2 real time counter
--*/

`include "ptpv2_defines.v"
`define WAVE_DUMP_FILE "./ptpv2.fst"

module tc_rtc;

  harness harness();
  //test use small intervals
  //defparam harness.ptpv2_endpoint.ptpv2_core_wrapper.ptpv2_core_inst.rtc_unit_inst.sync_io_inst.INT10MS = 24'd150000;        
  //defparam harness.ptpv2_endpoint.ptpv2_core_wrapper.ptpv2_core_inst.rtc_unit_inst.sync_io_inst.INTQ8MS = 24'd150000;        
  //defparam harness.lp_ptpv2_endpoint.ptpv2_core_wrapper.ptpv2_core_inst.rtc_unit_inst.sync_io_inst.INT10MS = 24'd150000;        
  //defparam harness.lp_ptpv2_endpoint.ptpv2_core_wrapper.ptpv2_core_inst.rtc_unit_inst.sync_io_inst.INTQ8MS = 24'd150000;        

  initial begin
    fork
      harness.ptpv2_endpoint.clkgen.reset;
      harness.lp_ptpv2_endpoint.clkgen.reset; 
    join   
    #700;

    //initial rtc 
    $display("initialize rtc with second offset 32'h1234_5678 !");
    harness.ptpv2_endpoint.ptp_agent.write_reg({`RTC_BLK_ADDR, `SC_OFST_ADDR0}, {16'b0, 16'h0});
    harness.ptpv2_endpoint.ptp_agent.write_reg({`RTC_BLK_ADDR, `SC_OFST_ADDR1}, 32'h1234_5678);
    harness.ptpv2_endpoint.ptp_agent.write_reg({`RTC_BLK_ADDR, `NS_OFST_ADDR}, 32'h3000_0000);
    harness.ptpv2_endpoint.ptp_agent.write_reg({`RTC_BLK_ADDR, `RTC_CTL_ADDR}, 32'h1);
    #10_0000;

    //clear rtc 
    $display("clear rtc !");
    harness.ptpv2_endpoint.ptp_agent.write_reg({`RTC_BLK_ADDR, `RTC_CTL_ADDR}, 32'h2);
    #10_0000;

    //set tick_inc value
    //xge clock T = 6.4ns, 6.4 * 2^26 = 429496729.6, convert to
    //hexdecimal, 32'h1999_999a
    $display("set tick_inc value to 32'h1234_5678 ! \n");
    harness.ptpv2_endpoint.ptp_agent.write_reg({`RTC_BLK_ADDR, `TICK_INC_ADDR}, 32'h1999_999a);
    #10_0000;

    //wait 2.2 seconds
    $display("wait 2.2 seconds for pps output!");

    #10_0000_0000;
    $display("1 second elapsed!");

    #10_0000_0000;
    $display("2 seconds elapsed!");

    #1_0000_0000;

    $finish;
  end

  //monitor pps output
  wire pps_out = harness.ptpv2_endpoint.ptpv2_core_wrapper.ptpv2_core_inst.pps_o;
  wire [79:0] rtc_std = harness.ptpv2_endpoint.ptpv2_core_wrapper.ptpv2_core_inst.rtc_std; 
  wire [79:0] pts_std = harness.ptpv2_endpoint.ptpv2_core_wrapper.ptpv2_core_inst.rtc_unit_inst.rtc_rgs_inst.pts_std_i;

  initial begin
    force harness.ptpv2_endpoint.ptpv2_core_wrapper.ptpv2_core_inst.pps_i = pps_out;
  end
  
  always @(posedge pps_out) begin
    #200;
    $display("positive edge of pps output is detected!");
    $display("current ptpv2 rtc time = %h", rtc_std);
    $display("\n");
    #200;
    $display("timestamp of pps input = %h", pts_std);
    $display("\n\n");
  end


  initial
  begin
    $dumpfile(`WAVE_DUMP_FILE);
    $dumpvars(0, tc_rtc.harness.ptpv2_endpoint);
    $dumpon;
    //$dumpoff;
  end    
endmodule
