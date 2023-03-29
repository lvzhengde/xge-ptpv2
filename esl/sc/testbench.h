#include <systemc.h> 
#include <verilated.h>

#include "MyBus.h"

// Include model header, generated from Verilating "ptp_top.v"
#include "Vptp_top.h"
#include "Vchannel_model.h"

SC_MODULE(testbench)
{ 
  Vptp_top* pTop;
  Vchannel_model* pChannel;

  sc_clock clk; //{"clk", 10, SC_NS, 0.5, 3, SC_NS, true};	

  sc_clock* ptx_clk;
  sc_signal<bool> tx_rst_n;
  sc_signal<uint64_t> xge_txd;
  sc_signal<uint32_t> xge_txc;

  sc_clock* prx_clk;
  sc_signal<bool> rx_rst_n;
  sc_signal<uint64_t> xge_rxd;
  sc_signal<uint32_t> xge_rxc;

  sc_clock* pbus2ip_clk;
  sc_signal<bool> bus2ip_rst_n;
  sc_signal<uint32_t> bus2ip_addr;
  sc_signal<uint32_t> bus2ip_data;
  sc_signal<bool> bus2ip_rd_ce; 
  sc_signal<bool> bus2ip_wr_ce; 
  sc_signal<uint32_t> ip2bus_data;   

  sc_signal<bool> int_ptp;
  sc_clock* prtc_clk;
  sc_signal<bool> rtc_rst_n;
  sc_signal<bool> pps_in;
  sc_signal<bool> pps_out;

  SC_CTOR(testbench):
	clk("clk", 6.4, SC_NS, 0.5, 2, SC_NS, true)
	{ 
    //clock decision
    ptx_clk = &clk;
    prx_clk = &clk;
    pbus2ip_clk = &clk;
    prtc_clk = &clk;

    // Construct the Verilated model, from Vptp_top.h generated from Verilating "ptp_top.v"
    pTop = new Vptp_top{"ptp_top"};
    pTop->tx_clk(*ptx_clk);
    pTop->tx_rst_n(tx_rst_n);
    pTop->xge_txd_o(xge_txd);
    pTop->xge_txc_o(xge_txc);

    pTop->rx_clk(*prx_clk);
    pTop->rx_rst_n(rx_rst_n);
    pTop->xge_rxd_i(xge_rxd);
    pTop->xge_rxc_i(xge_rxc);

    pTop->bus2ip_clk(*pbus2ip_clk);   
    pTop->bus2ip_rst_n(bus2ip_rst_n); 
    pTop->bus2ip_addr_i(bus2ip_addr);
    pTop->bus2ip_data_i(bus2ip_data);
    pTop->bus2ip_rd_ce_i(bus2ip_rd_ce);   
    pTop->bus2ip_wr_ce_i(bus2ip_wr_ce);  
    pTop->ip2bus_data_o(ip2bus_data);   

    pTop->int_ptp_o(int_ptp);
    pTop->rtc_clk(*prtc_clk);
    pTop->rtc_rst_n(rtc_rst_n);    
    pTop->pps_i(pps_in);        
    pTop->pps_o(pps_out);         

    pChannel = new Vchannel_model("delay_channel");
    pChannel->clk(*ptx_clk);
    pChannel->xge_rxc_i(xge_txc);
    pChannel->xge_rxd_i(xge_txd);
    pChannel->xge_txc_o(xge_rxc);
    pChannel->xge_txd_o(xge_rxd);
    
    SC_THREAD(say_hello); 
  } 
 
  void say_hello();
	void clean();
};  

