/*++
//  ptpv2 timestamp unit 
--*/
`include "ptpv2_defines.v"

module timestamp_unit (
  input               rtc_clk,                //fast clock used to lock signals
  input               rtc_rst_n,              //async. reset, active low
  input               dis_ptpv2_i,            //disable ptpv2

  //rx interface
  input               rx_clk,
  input               rx_rst_n,
  input               rx_clk_en_i,            //for adapting to gmii/mii
  input  [63:0]       xge_rxd_i,
  input  [7:0]        xge_rxc_i,
  output [63:0]       xge_rxd_o,
  output [7:0]        xge_rxc_o,
    
  //tx interface
  input               tx_clk,
  input               tx_rst_n,
  input               tx_clk_en_i,           //for adapting to gmii/mii
  input  [63:0]       xge_txd_i,
  input  [7:0]        xge_txc_i,
  output [63:0]       xge_txd_o,
  output [7:0]        xge_txc_o,

  //time input
  input  [79:0]       rtc_std_i,             //48 bits seconds + 32 bits nanoseconds
  input  [15:0]       rtc_fns_i,             //16 bit fractional nanoseconds of rtc

  //32 bits on chip bus access interface
  input               bus2ip_clk   ,
  input               bus2ip_rst_n  ,
  input  [31:0]       bus2ip_addr_i ,
  input  [31:0]       bus2ip_data_i ,
  input               bus2ip_rd_ce_i ,         //active high
  input               bus2ip_wr_ce_i ,         //active high
  output [31:0]       ip2bus_data_o ,  

  //interrupt signals generated from ptpv2 core
  output              int_rx_ptp_o,
  output              int_tx_ptp_o
);

  parameter BLK_ADDR = `TSU_BLK_ADDR;

endmodule


