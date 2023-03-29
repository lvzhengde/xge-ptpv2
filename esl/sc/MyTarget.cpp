/*****************************************************************************
 * Implements LT target
 */

#include "MyTarget.h"                      // our header
#include "reporting.h"                            // reporting macros

using namespace  std;

static const char *filename = "MyTarget.cpp"; ///< filename for reporting

SC_HAS_PROCESS(MyTarget);
///Constructor
MyTarget::MyTarget
( sc_core::sc_module_name module_name               // module name
, const unsigned int        ID                      // target ID
, const sc_core::sc_time    accept_delay            // accept delay (SC_TIME)
)
: sc_module               (module_name)             /// init module name
, m_ID                    (ID)                      /// init target ID
, m_accept_delay          (accept_delay)            /// init accept delay
{

  m_memory_socket.register_b_transport(this, &MyTarget::custom_b_transport);

  // register thread process
  SC_THREAD(reset_pbus);           
  sensitive << bus2ip_rst_n.neg();
}

//++
//peripheral bus operation functions
//--

// thread to initialize and reset peripheral bus
void MyTarget::reset_pbus(void)
{
  //initialize
  bus2ip_rd_ce_o.write(0);
  bus2ip_wr_ce_o.write(0);
  bus2ip_addr_o.write(0);
  bus2ip_data_o.write(0);

  //reset process
  for(;;)
  {
    wait();   //Resume on negative edge of bus2ip_rst_n
    if(bus2ip_rst_n.read() == 0)
    {
      bus2ip_rd_ce_o.write(0);
      bus2ip_wr_ce_o.write(0);
      bus2ip_addr_o.write(0);
      bus2ip_data_o.write(0);
    }
  }
}

// task to write register
void MyTarget::write_reg(const uint32_t addr, const uint32_t data)
{
  bus2ip_wr_ce_o.write(0);
  bus2ip_addr_o.write(0);
  bus2ip_data_o.write(0);
  
  wait(bus2ip_clk.posedge_event());
  bus2ip_wr_ce_o.write(1);
  bus2ip_addr_o.write(addr);
  bus2ip_data_o.write(data);
  
  wait(bus2ip_clk.posedge_event());
  wait(bus2ip_clk.posedge_event());
  
  bus2ip_wr_ce_o.write(0);
  bus2ip_addr_o.write(0);
  bus2ip_data_o.write(0);
}

// task to read register
void MyTarget::read_reg(const uint32_t addr, uint32_t &data)
{
  bus2ip_rd_ce_o.write(0);
  bus2ip_addr_o.write(0);
  
  wait(bus2ip_clk.posedge_event());
  bus2ip_rd_ce_o.write(1);
  bus2ip_addr_o.write(addr);
  
  wait(bus2ip_clk.posedge_event());
  wait(bus2ip_clk.posedge_event());
  wait(bus2ip_clk.posedge_event());

  data = ip2bus_data_i.read();
  
  wait(bus2ip_clk.posedge_event());
  bus2ip_rd_ce_o.write(0);
  bus2ip_addr_o.write(0);
} 

//==============================================================================
//  b_transport implementation calls from initiators
//
//=============================================================================
void
MyTarget::custom_b_transport
( tlm::tlm_generic_payload  &payload                // ref to  Generic Payload
, sc_core::sc_time          &delay_time             // delay time
)
{

  std::ostringstream  msg;
  msg.str("");
  sc_core::sc_time      mem_op_time;

  //m_target_memory.operation(payload, mem_op_time);

  delay_time = delay_time + m_accept_delay + mem_op_time;

  msg << "Target: " << m_ID
      << " Forcing a synch in a temporal decoupled initiator with wait( "
      << delay_time << "),";
  REPORT_INFO(filename,  __FUNCTION__, msg.str());

  //wait(delay_time);

  //delay_time = sc_core::SC_ZERO_TIME;

  msg.str("");
  msg << "Target: " << m_ID
      << " return from wait will return a delay of "
      << delay_time;
  REPORT_INFO(filename,  __FUNCTION__, msg.str());

  return;
}
