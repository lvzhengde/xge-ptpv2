/*++
//  tx frame reconstruction including one-step clock processing for ptpv2, etc.
--*/

`include "ptpv2_defines.v"

module tx_rcst (
  input               tx_clk,
  input               tx_rst_n,
  input               tx_clk_en_i,            //for adapting to gmii/mii

  input  [63:0]       txd_i,
  input  [7:0]        txc_i,

  output [63:0]       txd_o,
  output [7:0]        txc_o,

  //configuration register i/f
  input  [31:0]       tsu_cfg_i,

  //timestamp input
  input  [79:0]       sfd_timestamp_i,         //48 bits seconds + 32 bits nanoseconds
  input  [15:0]       sfd_timestamp_frac_ns_i, //16 bit fractional nanoseconds 

  output [63:0]       correctionField_i,       //ns * 2^16                               
  output [31:0]       ingress_time_i,          //32 bits ns  
  
  input  [10:0]       ptp_addr_base_i,
  input               is_ptp_message_i,  
  input  [15:0]       ptp_messageLength_i, 
  input  [3:0]        ptp_messageType_i,
  input  [15:0]       ptp_flagField_i,  
  
  input               ipv6_flag_i,
  input  [10:0]       ipv6_addr_base_i,
  input               ipv4_flag_i,  
  input  [10:0]       ipv4_addr_base_i,

  input  [10:0]       eth_count_base_i,
  input               get_sfd_done_i
);
  genvar i;


endmodule

