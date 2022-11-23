`ifndef PTPV2_DEFINES
`define PTPV2_DEFINES

//rtc related macros
`define FNS_W  (26)               //width of fractional nanosecond(16 < FNS_W < 28)
`define NSC_W  (`FNS_W + 32)       //width of nanosecond counter (ns+fractional ns)
`define SC2NS  (32'd1000000000)   //1 seconds = 10^9 nanoseconds


`endif

