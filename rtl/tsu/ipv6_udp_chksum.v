/*++
//  calculate ipv6 udp checksum then output the last two padding octets
--*/

`include "ptpv2_defines.v"

module ipv6_udp_chksum(
  input               tx_clk,
  input               tx_rst_n,              //async reset, active low
  input               tx_clk_en_i,

  input               ipv6_udp_chk_en_i,     
  input  [10:0]       eth_count_base_i,
  input               ipv6_flag_i,
  input  [10:0]       ipv6_addr_base_i,
  input  [10:0]       ptp_addr_base_i,
  input               is_ptp_message_i,

  input  [63:0]       txd_i,
  input  [7:0]        txc_i,

  input  [15:0]       ptp_messageLength_i,
  input  [3:0]        ptp_messageType_i,

  output reg          ipv6_padchg_flag_o,   //change udp padding octets or not
  output reg [10:0]   chkpad_addr_base_o,
  output reg [15:0]   chksum_pad_o
);
  genvar i;

  wire  chksum_calc_en = ipv6_udp_chk_en_i & ipv6_flag_i;

  //base address of ipv6 udp padding octets 
  always @(posedge tx_clk or negedge tx_rst_n) begin
    if(!tx_rst_n)
      chkpad_addr_base_o <= 11'h0;
    else if(tx_clk_en_i) begin
      if(chksum_calc_en == 1'b1)
        chkpad_addr_base_o <= ptp_addr_base_i + ptp_messageLength_i;
      else
        chkpad_addr_base_o <= 11'h0;
    end
  end

  //change flag of padding octets
  always @(posedge tx_clk or negedge tx_rst_n) begin
    if(!tx_rst_n)
      ipv6_padchg_flag_o <= 0;
    else if(tx_clk_en_i) begin
      if(chksum_calc_en == 1'b1 && is_ptp_message_i == 1'b1 && ptp_messageType_i[3] == 1'b0)
        ipv6_padchg_flag_o <= 1;
      else
        ipv6_padchg_flag_o <= 0;
    end
  end

  //simply detect the end of frame, 1 cycle pulse
  reg      simple_efd, simple_efd_z1;
  always @(*) begin
    simple_efd = 0;

    if((txd_i[`LANE0] == `TERMINATE && txc_i[0]) || (txd_i[`LANE1] == `TERMINATE && txc_i[1]) ||
	     (txd_i[`LANE2] == `TERMINATE && txc_i[2]) || (txd_i[`LANE3] == `TERMINATE && txc_i[3]) ||
       (txd_i[`LANE4] == `TERMINATE && txc_i[4]) || (txd_i[`LANE5] == `TERMINATE && txc_i[5]) ||
	     (txd_i[`LANE6] == `TERMINATE && txc_i[6]) || (txd_i[`LANE7] == `TERMINATE && txc_i[7])) 
	  simple_efd = 1; 
  end

  always @(posedge tx_clk or negedge tx_rst_n) begin
    if(!tx_rst_n)
      simple_efd_z1 <= 0;
    else if(tx_clk_en_i) 
      simple_efd_z1 <= simple_efd;
  end

endmodule  

