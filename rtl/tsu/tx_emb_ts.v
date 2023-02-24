
/*++
//  extract ingress timestamp (ns part) from messageTypeSpecific field
//  of ptpv2 event messages, recalculate correctionField. 
//  fill messageTypeSpecific field to zero according to standard.
--*/
`include "ptpv2_defines.v"

module tx_emb_ts(
  //xgmii interface
  input               tx_clk,
  input               tx_rst_n,
  input               tx_clk_en_i,            //for adapting to gmii/mii

  input  [63:0]       txd_i,
  input  [7:0]        txc_i,
  output reg [63:0]   txd_o,
  output reg [7:0]    txc_o,

  //configuration register i/f
  input  [31:0]       tsu_cfg_i,
  input  [31:0]       egress_asymmetry_i,

  //ptpv2 message related information
  input  [10:0]       eth_count_base_i,      //aligned with txd_i now
  input  [10:0]       ptp_addr_base_i,
  input  [3:0]        ptp_messageType_i,          
  input  [63:0]       ptp_correctionField_i,
  input  [31:0]       ptp_messageTypeSpecific_i,
  input               is_ptp_message_i,  
  input  [15:0]       ptp_messageLength_i,
  input  [15:0]       ptp_flagField_i, 

  input               ipv6_flag_i,
  input  [7:0]        ipv6_addr_base_i,
  input               ipv4_flag_i,
  input  [7:0]        ipv4_addr_base_i,

  //output information of currently sending packet
  output reg [63:0]   correctionField_o,     //ns * 2^16                               
  output reg [31:0]   ingress_time_o,        //32 bits ns  

  output reg [7:0]    ptp_addr_base_o,
  output reg [3:0]    ptp_messageType_o,          
  output reg          is_ptp_message_o,  
  output reg [15:0]   ptp_messageLength_o, 
  output reg [15:0]   ptp_flagField_o, 

  output reg          ipv6_flag_o,
  output reg [7:0]    ipv6_addr_base_o,
  output reg          ipv4_flag_o,
  output reg [7:0]    ipv4_addr_base_o,

  input               get_sfd_done_i,
  output reg          get_sfd_done_o,

  output reg [10:0]   eth_count_base_o
);
  genvar i;

  //xgmii delayline
  reg  [7:0]  txc_d1, txc_d2, txc_d3, txc_d4, txc_d5;
  reg  [63:0] txd_d1, txd_d2, txd_d3, txd_d4, txd_d5;

  always @(posedge tx_clk or negedge tx_rst_n) begin
    if(!tx_rst_n) begin
      {txc_d1, txc_d2, txc_d3, txc_d4, txc_d5} <= {5{8'b0}};
      {txd_d1, txd_d2, txd_d3, txd_d4, txd_d5} <= {5{64'b0}};
    end
    else if(tx_clk_en_i) begin
      {txc_d1, txc_d2, txc_d3, txc_d4, txc_d5} <= {txc_i, txc_d1, txc_d2, txc_d3, txc_d4};
      {txd_d1, txd_d2, txd_d3, txd_d4, txd_d5} <= {txd_i, txd_d1, txd_d2, txd_d3, txd_d4};
    end
  end

  reg  get_sfd_done_z1, get_sfd_done_z2, get_sfd_done_z3, get_sfd_done_z4, get_sfd_done_z5;

  always @(posedge tx_clk or negedge tx_rst_n) begin
    if(!tx_rst_n)
      {get_sfd_done_z1, get_sfd_done_z2, get_sfd_done_z3, get_sfd_done_z4, get_sfd_done_z5} <= {5{1'b0}};
    else if(tx_clk_en_i) 
      {get_sfd_done_z1, get_sfd_done_z2, get_sfd_done_z3, get_sfd_done_z4, get_sfd_done_z5} <= {get_sfd_done_i, 
                        get_sfd_done_z1, get_sfd_done_z2, get_sfd_done_z3, get_sfd_done_z4};
  end

  wire next_frame_detected = get_sfd_done_z3 && (~get_sfd_done_z4);

  //eth_count_base delay line
  reg  [10:0]  eth_count_base_z1, eth_count_base_z2, eth_count_base_z3, eth_count_base_z4, eth_count_base_z5;  
  always @(posedge tx_clk or negedge tx_rst_n) begin
    if(!tx_rst_n) begin
       {eth_count_base_z1, eth_count_base_z2, eth_count_base_z3, eth_count_base_z4, eth_count_base_z5} <= {5{11'h0}};
    end
    else if(tx_clk_en_i) begin
       {eth_count_base_z1, eth_count_base_z2, eth_count_base_z3, eth_count_base_z4, eth_count_base_z5} <= {eth_count_base_i, 
                           eth_count_base_z1, eth_count_base_z2, eth_count_base_z3, eth_count_base_z4};
    end
  end

  //ingress time and correctionField related
  wire one_step    = tsu_cfg_i[0];
  wire peer_delay  = tsu_cfg_i[2];
  wire tc_offload  = tsu_cfg_i[3];
  wire eg_asym_en  = tsu_cfg_i[6]
  wire emb_ingressTime_en = tsu_cfg_i[5];

  wire [63:0] asym_correction = {{16{egress_asymmetry_i[31]}}, egress_asymmetry_i[31:0], 16'b0};  

  always @(posedge tx_clk or negedge tx_rst_n) begin
    if(!tx_rst_n)
      correctionField_o <= 64'h0;
    else if(tx_clk_en_i) begin
      if(eg_asym_en == 1'b1 && (ptp_messageType_i[3:0] == 4'h1 || ptp_messageType_i[3:0] ==  4'h2) && is_ptp_message_i == 1'b1)
        correctionField_o <= ptp_correctionField_i - asym_correction;  // only for delay_req and pdelay_req messages.
      else
        correctionField_o <= ptp_correctionField_i;
    end
  end

  always @(posedge tx_clk or negedge tx_rst_n) begin
    if(!tx_rst_n)
      ingress_time_o <= 32'h0;
    else if(tx_clk_en_i) 
      ingress_time_o <= ptp_messageTypeSpecific_i;
  end

endmodule
