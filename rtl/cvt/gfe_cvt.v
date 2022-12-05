/*++
//  convert gmii/mii to/from xgmii-like interface
//  notes: the frame preamble octets must be 7
//         plus one octet sfd 0xd5
//  --*/
`include "ptpv2_defines.v"

module gfe_cvt (
  input                clk  ,
  input                rst_n,

  input                mii_mode_i,  //0:ge, 1: 100m/10m ethernet
  output reg           clk_en_o  ,

  //gmii/mii input
  input                en_i,
  input                er_i,
  input [7:0]          d_i ,
  
  //gmii/mii output converted from xgmii input
  output reg           en_o,
  output reg           er_o,
  output reg [7:0]     d_o ,   

  //xgmii input
  input [63:0]         xd_i,
  input [7:0]          xc_i,
  
  //xgmii output converted from gmii/mii input
  output reg [63:0]    xd_o,
  output reg [7:0]     xc_o
);

  //generate clock enable signal
  wire [3:0] clk_cnt_p1;
  reg  [3:0] clk_cnt;

  assign clk_cnt_p1 = clk_cnt + 1;
  always @(posedge clk or negedge rst_n) begin
    if(!rst_n) 
      clk_cnt <= 4'd0;
    else if((mii_mode_i == 0 && clk_cnt == 4'd7)||(mii_mode_i == 0 && clk_cnt == 4'd15))
      clk_cnt <= 4'd0;
    else 
      clk_cnt <= clk_cnt_p1;
  end

  always @(posedge clk or negedge rst_n) begin
    if(!rst_n) 
      clk_en_o <= 0;
    else if((mii_mode_i == 0 && clk_cnt == 4'd7)||(mii_mode_i == 0 && clk_cnt == 4'd15))
      clk_en_o <= 1;
    else 
      clk_en_o <= 0;
  end

  //++
  //convert gmii/mii signals to xgmii-like signals
  //--
  reg         en_d1;
  reg         er_d1;
  reg [7:0]   d_d1;

  always @(posedge ) begin
    en_d1 <= en_i;   
    er_d1 <= er_i;
    d_d1  <= d_i ;
  end

  //ge to xgmii conversion
  reg [63:0]    ge_xd;
  reg [7:0]     ge_xc;

  wire ge_frm_start = en_i & (~en_d1);
  wire ge_frm_end = en_d1 & (~en_i);
  wire ge_sfd = (d_i[7:0] == 8'hd5);
  reg  ge_got_sfd;

  always @(posedge clk or negedge rst_n) begin
    if(!rst_n) 
      ge_got_sfd <= 0;
    else if(ge_sfd == 1 && en_i == 1)
      ge_got_sfd <= 1;
    else if(ge_frm_start || ge_frm_end)
      ge_got_sfd <= 0;
  end

  reg  [7:0] ge_octets;
  reg        ge_control;

  always @(*) begin 
    ge_xd = xd_o;
    ge_xc = xc_o;

    if(en_i && ge_frm_start) begin
      ge_control = 1;
      ge_octets = `START;
    end
    else if(en_i == 1 && er_i == 0) begin
      ge_control = 0;
      ge_octets = d_i;
    end
    else if(ge_frm_end) begin
      ge_control = 1;
      ge_octets = `TERMINATE;
    end
    else if(er_i == 1) begin
      ge_control = 1;
      ge_octets = `ERROR;
    end
    else begin
      ge_control = 1;
      ge_octets = `IDLE;
    end

    case(clk_cnt)
      4'd0:
      begin

      end
      4'd1:
      begin

      end

    endcase
  end

endmodule
