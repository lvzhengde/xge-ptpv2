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
  output reg          int_rx_ptp_o
);
  genvar i;   

  //xgmii input delay line
  reg  [63:0]         rxd_z1, rxd_z2, rxd_z3, rxd_z4, rxd_z5, rxd_z6;
  reg  [7:0]          rxc_z1, rxc_z2, rxc_z3, rxc_z4, rxc_z5, rxc_z6;

  always @(posedge rx_clk or negedge rx_rst_n) begin
    if(!rx_rst_n) begin
       {rxd_z1, rxd_z2, rxd_z3, rxd_z4, rxd_z5, rxd_z6} <= {6{64'h0}};
       {rxc_z1, rxc_z2, rxc_z3, rxc_z4, rxc_z5, rxc_z6} <= {6{8'h0}};
    end
    else if(rx_clk_en_i) begin
       {rxd_z1, rxd_z2, rxd_z3, rxd_z4, rxd_z5, rxd_z6} <= {rxd_i, rxd_z1, rxd_z2, rxd_z3, rxd_z4, rxd_z5}; 
       {rxc_z1, rxc_z2, rxc_z3, rxc_z4, rxc_z5, rxc_z6} <= {rxc_i, rxc_z1, rxc_z2, rxc_z3, rxc_z4, rxc_z5}; 
    end
  end

  //++
  //detect start of frame and end of frame
  //--
  reg  [7:0] is_sfd;
  reg  [7:0] is_start, is_start_z1;

  always @(posedge rx_clk or negedge rx_rst_n) begin 
    if(!rx_rst_n) 
      is_start_z1 <= 8'h0;
    else if(rx_clk_en_i) 
      is_start_z1 <= is_start;
  end

  generate
    for(i = 0; i < 8; i = i+1) begin
      always @(*) begin
        is_start[i] = 0;
        is_sfd[i] = 0;

        if(rxd_i[8*i+7: 8*i] == `START && rxc_i[i] == 1)
          is_start[i] = 1;
        if(rxd_i[8*i+7: 8*i] == `SFD && rxc_i[i] == 0)
          is_sfd[i] = 1;
      end
    end //for i
  endgenerate

  reg    get_sfd_done_p1, get_sfd_done;
  reg    get_sfd_done_z1, get_sfd_done_z2, get_sfd_done_z3, get_sfd_done_z4, get_sfd_done_z5;
  reg    get_efd_done_p1, get_efd_done;
  reg    get_efd_done_z1, get_efd_done_z2, get_efd_done_z3, get_efd_done_z4, get_efd_done_z5;

  //start of frame
  always @(*) begin
    if(get_efd_done_p1 && get_efd_done)
      get_sfd_done_p1 = 0;
    else if((|is_sfd) && ((|is_start) || (|is_start_z1)))
      get_sfd_done_p1 = 1;
    else
      get_sfd_done_p1 = get_sfd_done;
  end
  
  always @(posedge rx_clk or negedge rx_rst_n) begin 
    if(!rx_rst_n) 
      get_sfd_done <= 0;
    else if(rx_clk_en_i) 
      get_sfd_done <= get_sfd_done_p1;
  end

  always @(posedge rx_clk or negedge rx_rst_n) begin
    if(!rx_rst_n)
      {get_sfd_done_z1, get_sfd_done_z2, get_sfd_done_z3, get_sfd_done_z4, get_sfd_done_z5} <= {5{1'b0}};
    else if(rx_clk_en_i)
      {get_sfd_done_z1, get_sfd_done_z2, get_sfd_done_z3, get_sfd_done_z4, get_sfd_done_z5} <= {get_sfd_done, 
                        get_sfd_done_z1, get_sfd_done_z2, get_sfd_done_z3, get_sfd_done_z4}; 
  end
  
  //end of frame
  always @(*) begin
    if((rxd_i[`LANE0] == `TERMINATE && rxc_i[0]) || (rxd_i[`LANE1] == `TERMINATE && rxc_i[1]) ||
	     (rxd_i[`LANE2] == `TERMINATE && rxc_i[2]) || (rxd_i[`LANE3] == `TERMINATE && rxc_i[3]) ||
       (rxd_i[`LANE4] == `TERMINATE && rxc_i[4]) || (rxd_i[`LANE5] == `TERMINATE && rxc_i[5]) ||
	     (rxd_i[`LANE6] == `TERMINATE && rxc_i[6]) || (rxd_i[`LANE7] == `TERMINATE && rxc_i[7])) 
	    get_efd_done_p1 = 1; 
    else if((rxd_i[`LANE0] == `START && rxc_i[0]) || (rxd_i[`LANE1] == `START && rxc_i[1]) ||
	          (rxd_i[`LANE2] == `START && rxc_i[2]) || (rxd_i[`LANE3] == `START && rxc_i[3]) ||
            (rxd_i[`LANE4] == `START && rxc_i[4]) || (rxd_i[`LANE5] == `START && rxc_i[5]) ||
	          (rxd_i[`LANE6] == `START && rxc_i[6]) || (rxd_i[`LANE7] == `START && rxc_i[7])) 
	    get_efd_done_p1 = 0; 
    else
      get_efd_done_p1 = get_efd_done;
  end

  always @(posedge rx_clk or negedge rx_rst_n) begin 
    if(!rx_rst_n) 
      get_efd_done <= 0;
    else if(rx_clk_en_i) 
      get_efd_done <= get_efd_done_p1;
  end

  always @(posedge rx_clk or negedge rx_rst_n) begin
    if(!rx_rst_n)
      {get_efd_done_z1, get_efd_done_z2, get_efd_done_z3, get_efd_done_z4, get_efd_done_z5} <= {5{1'b0}};
    else if(rx_clk_en_i)
      {get_efd_done_z1, get_efd_done_z2, get_efd_done_z3, get_efd_done_z4, get_efd_done_z5} <= {get_efd_done, 
                        get_efd_done_z1, get_efd_done_z2, get_efd_done_z3, get_efd_done_z4}; 
  end

  //++
  //ethernet octet count at lane 0
  //--
 reg  [10:0]   eth_count_base, eth_count_base_z1, eth_count_base_z2, eth_count_base_z3, 
               eth_count_base_z4, eth_count_base_z5;
 
  //note: the eth_count of the first octet after SFD is set to 8 
  always @(posedge rx_clk or negedge rx_rst_n) begin
    if(!rx_rst_n)
      eth_count_base <= 0;
    else if(rx_clk_en_i) begin
      if(get_sfd_done_p1 == 1 && get_sfd_done == 0) begin
        if(is_sfd[0])
          eth_count_base <= 7;
        else if(is_sfd[1])
          eth_count_base <= 6;
        else if(is_sfd[2])    
          eth_count_base <= 5;
        else if(is_sfd[3])    
          eth_count_base <= 4;
        else if(is_sfd[4])    
          eth_count_base <= 3;
        else if(is_sfd[5])    
          eth_count_base <= 2;
        else if(is_sfd[6])    
          eth_count_base <= 1;
        else if(is_sfd[7])    
          eth_count_base <= 0;
      end
      else if(get_efd_done == 1 && get_efd_done_z1 == 0)
        eth_count_base <= 0;
      else if(get_sfd_done == 1) 
          eth_count_base <= eth_count_base + 8;
    end
  end

  //eth_count_base delay line
  always @(posedge rx_clk or negedge rx_rst_n) begin
    if(!rx_rst_n) begin
       {eth_count_base_z1, eth_count_base_z2, eth_count_base_z3, eth_count_base_z4, eth_count_base_z5} <= {5{11'h0}};
    end
    else if(rx_clk_en_i) begin
       {eth_count_base_z1, eth_count_base_z2, eth_count_base_z3, eth_count_base_z4, eth_count_base_z5} <= {eth_count_base, 
        eth_count_base_z1, eth_count_base_z2, eth_count_base_z3, eth_count_base_z4};
    end
  end

  //rx timestamp plane trigger signal
  always @(posedge rx_clk or negedge rx_rst_n) begin
    if(!rx_rst_n)
      rxts_trig_o <= 0;
    else if(rx_clk_en_i)
      rxts_trig_o <= (get_sfd_done_p1 == 1'b1 && get_sfd_done == 1'b0);                                                         
  end
  
  //++
  //parse layer 2 ethernet header
  //--
  reg  [47:0]      mac_da;
  reg  [47:0]      mac_sa;
  reg  [15:0]      length_type;
  reg  [15:0]      vlan1_length_type;
  reg  [15:0]      vlan2_length_type;

  reg  [47:0]      mac_da_lane[7:0];
  reg  [47:0]      mac_sa_lane[7:0];
  reg  [15:0]      length_type_lane[7:0];
  reg  [15:0]      vlan1_length_type_lane[7:0];
  reg  [15:0]      vlan2_length_type_lane[7:0];

  generate
    for(i = 0; i < 8; i = i+1) begin : PARSE_ETHERNET_HEADER
      reg [10:0] eth_count;
      reg [7:0]  rxd_lane;

      reg [47:0] mac_da_tmp;
      reg [47:0] mac_sa_tmp;
      reg [15:0] length_type_tmp;
      reg [15:0] vlan1_length_type_tmp;
      reg [15:0] vlan2_length_type_tmp;

      always @(*) begin
        rxd_lane = 8'h0;
        eth_count = 0;

        if(get_sfd_done == 1) begin
          eth_count = eth_count_base + i;
        end

        rxd_lane = rxd_z1[8*i+7: 8*i];
      end

      always @(*) begin
          mac_da_tmp = mac_da;
          mac_sa_tmp = mac_sa;
          length_type_tmp = length_type;
          vlan1_length_type_tmp = vlan1_length_type;
          vlan2_length_type_tmp = vlan2_length_type;

        if(get_sfd_done == 1 && (!rxc_z1[i])) begin
          if(eth_count == 11'd8)   mac_da_tmp[47:40]     = rxd_lane;
          if(eth_count == 11'd9)   mac_da_tmp[39:32]     = rxd_lane;        
          if(eth_count == 11'd10)  mac_da_tmp[31:24]     = rxd_lane;        
          if(eth_count == 11'd11)  mac_da_tmp[23:16]     = rxd_lane;        
          if(eth_count == 11'd12)  mac_da_tmp[15:8]      = rxd_lane;        
          if(eth_count == 11'd13)  mac_da_tmp[7:0]       = rxd_lane; 

          if(eth_count == 11'd14)  mac_sa_tmp[47:40]     = rxd_lane;
          if(eth_count == 11'd15)  mac_sa_tmp[39:32]     = rxd_lane;        
          if(eth_count == 11'd16)  mac_sa_tmp[31:24]     = rxd_lane;        
          if(eth_count == 11'd17)  mac_sa_tmp[23:16]     = rxd_lane;        
          if(eth_count == 11'd18)  mac_sa_tmp[15:8]      = rxd_lane;        
          if(eth_count == 11'd19)  mac_sa_tmp[7:0]       = rxd_lane;          
            
          if(eth_count == 11'd20)  length_type_tmp[15:8] = rxd_lane;
          if(eth_count == 11'd21)  length_type_tmp[7:0]  = rxd_lane; 
          
          if(eth_count == 11'd24)  vlan1_length_type_tmp[15:8] = rxd_lane;
          if(eth_count == 11'd25)  vlan1_length_type_tmp[7:0]  = rxd_lane; 
            
          if(eth_count == 11'd28)  vlan2_length_type_tmp[15:8] = rxd_lane;
          if(eth_count == 11'd29)  vlan2_length_type_tmp[7:0]  = rxd_lane;        
        end 
        else if(get_efd_done_z1 == 1) begin
          mac_da_tmp = 48'h0;
          mac_sa_tmp = 48'h0;
          length_type_tmp = 16'h0;
          vlan1_length_type_tmp = 16'h0;
          vlan2_length_type_tmp = 16'h0;
        end
      end //always

      always @(*) begin
        mac_da_lane[i] = mac_da_tmp;
        mac_sa_lane[i] = mac_sa_tmp;
        length_type_lane[i] = length_type_tmp;
        vlan1_length_type_lane[i] = vlan1_length_type_tmp;
        vlan2_length_type_lane[i] = vlan2_length_type_tmp;
      end
    end //for i
  endgenerate

  always @(posedge rx_clk or negedge rx_rst_n) begin
    if(!rx_rst_n) begin
      mac_da <= 48'h0;
      mac_sa <= 48'h0;
      length_type <= 16'h0;
      vlan1_length_type <= 16'h0;
      vlan2_length_type <= 16'h0;
    end
    else begin
      mac_da <= `OR_LANE(mac_da_lane);
      mac_sa <= `OR_LANE(mac_sa_lane);
      length_type <= `OR_LANE(length_type_lane);
      vlan1_length_type <= `OR_LANE(vlan1_length_type_lane);
      vlan2_length_type <= `OR_LANE(vlan2_length_type_lane);
    end
  end


endmodule
