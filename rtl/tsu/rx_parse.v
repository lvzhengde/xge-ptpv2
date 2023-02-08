/*++
//  rx frame parsing
//  start of frame should be : S Dp Dp Dp Dp Dp Dp SFD
--*/

`include "ptpv2_defines.v"

module rx_parse(
  //xgmii interface
  input               rx_clk,
  input               rx_rst_n,
  input               rx_clk_en_i,            //for adapting to gmii/mii
  input  [63:0]       rxd_i,
  input  [7:0]        rxc_i,
  output [63:0]       rxd_o,
  output [7:0]        rxc_o,

  //timestamp input
  input  [79:0]       sfd_timestamp_i,       //48 bits seconds + 32 bits nanoseconds
  input  [15:0]       sfd_timestamp_frac_ns, //16 bit fractional nanoseconds 

  //configuration register i/f
  input  [31:0]       tsu_cfg_i,
  input  [31:0]       link_delay_i,
  input  [31:0]       ingress_asymmetry_i,
  
  //timestamp i/f, sync to rtc_clk
  output reg          rxts_trig_o,
  output reg          rxts_valid_o,
  
  output reg [79:0]   rx_sourcePortIdentity_o,  
  output reg [15:0]   rx_flagField_o,
  output reg [15:0]   rx_seqId_o,                 
  output reg [3:0]    rx_versionPTP_o,
  output reg [3:0]    rx_minorVersionPTP_o,
  output reg [3:0]    rx_messageType_o,  
  output reg [3:0]    rx_majorSdoId_o,

  //ptpv2 rx interrupt signal 
  output              int_rx_ptp_o
);

endmodule
