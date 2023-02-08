/*++
//  tx frame parsing
//  start of frame should be : S Dp Dp Dp Dp Dp Dp SFD
--*/

`include "ptpv2_defines.v"

module tx_parse(
  //xgmii interface
  input               tx_clk,
  input               tx_rst_n,
  input               tx_clk_en_i,            //for adapting to gmii/mii
  input  [63:0]       txd_i,
  input  [7:0]        txc_i,
  output [63:0]       txd_o,
  output [7:0]        txc_o,

  //timestamp input
  input  [79:0]       sfd_timestamp_i,       //48 bits seconds + 32 bits nanoseconds
  input  [15:0]       sfd_timestamp_frac_ns, //16 bit fractional nanoseconds 

  //configuration register i/f
  input  [31:0]       tsu_cfg_i,
  input  [31:0]       egress_asymmetry_i,
  
  //timestamp i/f, sync to rtc_clk
  output reg          txts_trig_o,
  output reg          txts_valid_o,
  
  output reg [79:0]   tx_sourcePortIdentity_o,  
  output reg [15:0]   tx_flagField_o,
  output reg [15:0]   tx_seqId_o,                 
  output reg [3:0]    tx_versionPTP_o,
  output reg [3:0]    tx_minorVersionPTP_o,
  output reg [3:0]    tx_messageType_o,  
  output reg [3:0]    tx_majorSdoId_o,

  //ptpv2 tx interrupt signal 
  output              int_tx_ptp_o
);

endmodule

