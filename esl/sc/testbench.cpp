#include "testbench.h"
#include "reporting.h"                     // reporting macros

using namespace  std;

static const char *filename = "testbench.cpp"; ///< filename for reporting

SC_HAS_PROCESS(testbench);
///constructor
testbench::testbench 
( sc_core::sc_module_name name
, const unsigned int  sw_type                  ///< software type, 0: loopback test; 1: PTPd protocol test
) 
: sc_module               (name)               /// init module name
, m_sw_type               (sw_type)
//, clk("clk", 6.4, SC_NS, 0.5, 2, SC_NS, true)       
//, clk_lp("clk_lp", 6.4, SC_NS, 0.5, 2, SC_NS, true) 
{
  pInstance    = NULL;
  pInstance_lp = NULL;
  pChannel     = NULL;
  pChannel_lp  = NULL;

  if(sw_type == 0)
  {
    pInstance = new ptp_instance("ptp_instance", m_sw_type, 1);
    pChannel  = new Vchannel_model("delay_channel");
  
    //bind ptp_instance ports
    pInstance->bus2ip_clk    (clk  );
    pInstance->bus2ip_rst_n  (rst_n);
    pInstance->tx_clk        (clk  );
    pInstance->tx_rst_n      (rst_n);
    pInstance->xge_txc_o     (xge_txc);
    pInstance->xge_txd_o     (xge_txd);
    pInstance->rx_clk        (clk  );
    pInstance->rx_rst_n      (rst_n);
    pInstance->xge_rxc_i     (xge_rxc);
    pInstance->xge_rxd_i     (xge_rxd);
    pInstance->rtc_clk       (clk  );
    pInstance->rtc_rst_n     (rst_n);
    pInstance->pps_i         (pps_in);
    pInstance->pps_o         (pps_out);
    pInstance->proc_rst_n    (rst_n);

    //bind delay channel ports
    pChannel->clk(clk);
    pChannel->xge_rxc_i(xge_txc);
    pChannel->xge_rxd_i(xge_txd);
    pChannel->xge_txc_o(xge_rxc);
    pChannel->xge_txd_o(xge_rxd);

    //declare sc thread
    SC_THREAD(reset_gen); 
  }
}

///destructor
testbench::~testbench()
{
  if(pInstance    != NULL)
    delete pInstance;

  if(pInstance_lp != NULL)
    delete pInstance_lp; 

  if(pChannel     != NULL)
  {
    pChannel->final();
    delete pChannel; 
  }

  if(pChannel_lp  != NULL)
  {
    pChannel_lp->final();
    delete pChannel_lp; 
  }
}

///generate reset for local PTP instance
void testbench::reset_gen()
{
  //initialize to inactive
  rst_n.write(true); 

	wait(55, SC_NS);
  rst_n.write(false);

	wait(40*5000, SC_NS);
	wait(355, SC_NS);

  rst_n.write(true);
}

///generate reset for link partner PTP instance
void testbench::lp_reset_gen()
{
  //initialize to inactive
  lp_rst_n.write(true); 

	wait(55, SC_NS);
  lp_rst_n.write(false);

	wait(40*5000, SC_NS);
	wait(355, SC_NS);

  lp_rst_n.write(true);
}

