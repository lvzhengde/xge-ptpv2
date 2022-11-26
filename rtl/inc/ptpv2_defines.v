`ifndef PTPV2_DEFINES
`define PTPV2_DEFINES

//rtc related macros
`define FNS_W  (26)                //width of fractional nanosecond(16 < FNS_W < 28)
`define NSC_W  (`FNS_W + 32)       //width of nanosecond counter (ns+fractional ns)
`define SC2NS  (32'd1000000000)    //1 seconds = 10^9 nanoseconds

//rtc register addresses
`define RTC_BASE_ADDR    (24'h000000)

`define RTC_CTL_ADDR     (8'h00)
`define TICK_INC_ADDR    (8'h04)
`define NS_OFST_ADDR     (8'h08)
`define SC_OFST_ADDR0    (8'h0c)
`define SC_OFST_ADDR1    (8'h10)
`define CUR_TM_ADDR0     (8'h14)
`define CUR_TM_ADDR1     (8'h18)
`define CUR_TM_ADDR2     (8'h1c)
`define PTS_ADDR0        (8'h20)
`define PTS_ADDR1        (8'h24)
`define PTS_ADDR2        (8'h28)
`define PPS_W_ADDR       (8'h2c)



`timescale 1ns/10fs

`endif

