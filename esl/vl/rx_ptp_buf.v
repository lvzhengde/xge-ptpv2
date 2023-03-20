/*++
// rx ptp frame buffer 
// for simplicity, just behavioral
--*/

`include "ptpv2_defines.v"

module rx_ptp_buf (
  //rx interface
  input               rx_clk,
  input               rx_rst_n,

  input  [63:0]       xge_rxd_i,
  input  [7:0]        xge_rxc_i,

  //32 bits on chip bus access interface
  input               bus2ip_clk   ,
  input               bus2ip_rst_n  ,
  input  [31:0]       bus2ip_addr_i ,
  input  [31:0]       bus2ip_data_i ,
  input               bus2ip_rd_ce_i ,         //active high
  input               bus2ip_wr_ce_i ,         //active high
  output reg [31:0]   ip2bus_data_o   
);
  parameter RX_BUF_BADDR = 32'h1000;
  reg  [7:0]  rcvd_frame[511:0];
  reg  [31:0] rd_buf[127:0]; //buffer to bus
  reg  [9:0]  eth_count;
  reg  [8:0]  frm_len;
  reg         wr_fin, wr_fin_z1, wr_fin_z2;

  //receiving process, behavioral, using block assignment
  always @(posedge rx_clk or negedge rx_rst_n) begin : XGE_RCV_PROC
    integer i;

    if(!rx_rst_n) begin
      eth_count = 0;
      frm_len   = 0;
      wr_fin    = 0;
    end
    else begin
      for(i = 0; i < 8; i = i+1) begin
        if(xge_rxc_i[i] == 1 && xge_rxd_i[i*8+7-:8] == `START) begin 
          eth_count = 0;
          frm_len   = 0;
          wr_fin    = 0;
          rcvd_frame[eth_count[8:0]] = 8'h55;
          eth_count = eth_count + 1;
        end
        else if(xge_rxc_i[i] == 0  && eth_count <= 511) begin
          rcvd_frame[eth_count[8:0]] = xge_rxd_i[i*8+7-:8];      
          eth_count = eth_count + 1;
        end
        else if(xge_rxc_i[i] == 1 && xge_rxd_i[i*8+7-:8] == `TERMINATE) begin
          frm_len   = eth_count[8:0] - 8;
          wr_fin    = 1;
          eth_count = 0;
        end
      end //for i
    end //else
  end

  always @(posedge rx_clk) {wr_fin_z1, wr_fin_z2} <= {wr_fin, wr_fin_z1};

  //transfer received frame data to read buffer
  always @(posedge rx_clk) begin : TO_RD_BUFF
    reg [8:0] i;
    integer   j;
  
    if(wr_fin_z1 & (~wr_fin_z2)) begin
      j = 0;
      for(i = 8; i < frm_len+8; i = i+4) begin
        rd_buf[j] = {rcvd_frame[i+3], rcvd_frame[i+2], rcvd_frame[i+1], rcvd_frame[i]};
        j = j + 1;
      end //i
    end //if
  end

  //bus read operation
  wire [31:0] masked_addr  = bus2ip_addr_i & 32'h1ff;
  wire [6:0]  shifted_addr = masked_addr[8:2];

  always @(*) begin
    if(bus2ip_rd_ce_i == 1'b1 && bus2ip_addr_i == RX_BUF_BADDR + 32'h200) //frame length
      ip2bus_data_o[31:0] = {23'b0, frm_len};
    else if(bus2ip_rd_ce_i == 1'b1 && bus2ip_addr_i >= RX_BUF_BADDR && bus2ip_addr_i < (RX_BUF_BADDR+32'h200)) 
      ip2bus_data_o[31:0] = rd_buf[shifted_addr];
    else
      ip2bus_data_o[31:0] = 32'h0;
  end

endmodule

