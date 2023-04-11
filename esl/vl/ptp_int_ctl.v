/*++
// interrupt controller for xge-ptpv2 core 
--*/

module ptp_int_ctl (
  //32 bits on chip bus access interface
  input               bus2ip_clk   ,
  input               bus2ip_rst_n  ,
  input  [31:0]       bus2ip_addr_i ,
  input  [31:0]       bus2ip_data_i ,
  input               bus2ip_rd_ce_i ,         //active high
  input               bus2ip_wr_ce_i ,         //active high
  output reg [31:0]   ip2bus_data_o ,  

  //interrupt inputs
  input               intxms_i,
  input               int_rx_ptp_i,
  input               int_tx_ptp_i,

  //combined interrupt output
  output reg          int_ptp_o
);
  parameter INT_BASE_ADDR = 32'h300;

  //delay interrupt inputs
  reg  intxms_z1, intxms_z2, intxms_z3;
  reg  int_rx_ptp_z1, int_rx_ptp_z2, int_rx_ptp_z3;
  reg  int_tx_ptp_z1, int_tx_ptp_z2, int_tx_ptp_z3;

  always @(posedge bus2ip_clk or negedge bus2ip_rst_n) begin
    if(!bus2ip_rst_n) begin
      {intxms_z1, intxms_z2, intxms_z3} <= 3'b0;
      {int_rx_ptp_z1, int_rx_ptp_z2, int_rx_ptp_z3} <= 3'b0;
      {int_tx_ptp_z1, int_tx_ptp_z2, int_tx_ptp_z3} <= 3'b0;
    end
    else begin
      {intxms_z1, intxms_z2, intxms_z3} <= {intxms_i, intxms_z1, intxms_z2};
      {int_rx_ptp_z1, int_rx_ptp_z2, int_rx_ptp_z3} <= {int_rx_ptp_i, int_rx_ptp_z1, int_rx_ptp_z2};
      {int_tx_ptp_z1, int_tx_ptp_z2, int_tx_ptp_z3} <= {int_tx_ptp_i, int_tx_ptp_z1, int_tx_ptp_z2};
    end
  end

  //generate read clear signal internally
  reg  [31:0]      bus2ip_addr_z1, bus2ip_addr_z2 ; 
  reg              bus2ip_rd_ce_z1 ;         //active high
  reg              read_clear, read_clear_z1 ; 
  wire             read_clear_pulse = read_clear & (~read_clear_z1);

  always @(posedge bus2ip_clk or negedge bus2ip_rst_n) begin
    if(!bus2ip_rst_n) begin
      bus2ip_addr_z1 <= 32'h0;
      bus2ip_addr_z2 <= 32'h0;
      bus2ip_rd_ce_z1 <= 1'b0;
      read_clear_z1  <= 1'b0;
    end
    else begin
      bus2ip_addr_z1 <= bus2ip_addr_i;
      bus2ip_addr_z2 <= bus2ip_addr_z1;
      bus2ip_rd_ce_z1 <= bus2ip_rd_ce_i;
      read_clear_z1  <= read_clear  ;
    end
  end

  always @(posedge bus2ip_clk or negedge bus2ip_rst_n) begin
    if(!bus2ip_rst_n)
      read_clear <= 1'b0;
    else if(bus2ip_rd_ce_i == 1'b0 && bus2ip_rd_ce_z1 == 1'b1) //single read
      read_clear <= 1'b1;
    else if(bus2ip_addr_i != bus2ip_addr_z1 && bus2ip_rd_ce_i == 1'b1 && bus2ip_rd_ce_z1 == 1'b1) //continuous read and not the first one
      read_clear <= 1'b1;
    else if(read_clear_z1 == 1'b1)
      read_clear <= 1'b0;
  end

  //interrupt status register
  reg  [2:0]  int_status;

  always @(posedge bus2ip_clk or negedge bus2ip_rst_n) begin
    if(!bus2ip_rst_n)
      int_status[2:0] <= 3'b0;
    else if(read_clear_pulse == 1'b1 && bus2ip_addr_z2[31:0] == INT_BASE_ADDR) 
      int_status[2:0] <= 3'b0;
    else begin
      if(intxms_z2 & (~intxms_z3))           int_status[2] <= 1'b1;
      if(int_rx_ptp_z2 &(~int_rx_ptp_z3) )   int_status[1] <= 1'b1;
      if(int_tx_ptp_z2 &(~int_tx_ptp_z3) )   int_status[0] <= 1'b1;
    end
  end

  //interrupt mask register
  reg  [2:0]  int_mask;

  always @(posedge bus2ip_clk or negedge bus2ip_rst_n) begin
    if(!bus2ip_rst_n)
      int_mask[2:0] <= 3'b111;
    else if(bus2ip_wr_ce_i == 1'b1 && bus2ip_addr_i == (INT_BASE_ADDR+1))
      int_mask[2:0] <= bus2ip_data_i[2:0];
  end

  //bus read operation
  always @(*) begin
    if(bus2ip_rd_ce_i == 1'b1 && bus2ip_addr_i == INT_BASE_ADDR)
      ip2bus_data_o[31:0] = {29'b0, int_status[2:0]};
    else if(bus2ip_rd_ce_i == 1'b1 && bus2ip_addr_i == (INT_BASE_ADDR+1))
      ip2bus_data_o[31:0] = {29'b0, int_mask[2:0]};
    else
      ip2bus_data_o[31:0] = 32'h0;
  end

  //ouput interrupt
  always @(posedge bus2ip_clk or negedge bus2ip_rst_n) begin
    if(!bus2ip_rst_n)
      int_ptp_o <= 1'b0;
    else
      int_ptp_o <= |(int_status[2:0] & int_mask[2:0]);
  end

endmodule
