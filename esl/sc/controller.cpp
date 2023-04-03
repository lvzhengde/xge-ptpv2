/*
 * controller.cpp
 */

#include "reporting.h"               	 // reporting macros
#include "controller.h"                // controller declarations

using namespace std;

static const char *filename = "controller.cpp";  ///< filename for reporting

/// Constructor
SC_HAS_PROCESS(controller);

controller::controller            
( sc_core::sc_module_name name                  // instance name
, const unsigned int    ID                      // initiator ID
)

: sc_module           ( name              )     /// instance name
, m_ID                ( ID                )     /// initiator ID
{ 
  SC_THREAD(controller_thread);
  
  SC_THREAD(isr_thread);
  sensitive << int_ptp_i.pos();

  // build transaction pool 
  m_ptxn = new tlm::tlm_generic_payload;
}

/// Destructor
controller::~controller()
{
  delete m_ptxn;
}

/// SystemC thread for generation of GP traffic

void controller::controller_thread(void)
{
  std::ostringstream  msg;                      ///< log message
  
  msg.str ("");
  msg << "Initiator: " << m_ID << " Starting PTPd Application";
  REPORT_INFO(filename, __FUNCTION__, msg.str());
  
  tlm::tlm_generic_payload  *transaction_ptr;   ///< transaction pointer
  unsigned char             *data_buffer_ptr;   ///< data buffer pointer
  

  msg.str ("");
  msg << "Controller : " << m_ID << endl 
  << "=========================================================" << endl 
  << "            ####  PTPd Application Complete  #### ";
  REPORT_INFO(filename, __FUNCTION__, msg.str());
} // end controller_thread

//interrupt service routine thread 
void controller::isr_thread (void)
{

} 

// manipulate transaction through sc_fifo in/out interface
void controller::transaction_manip(tlm::tlm_generic_payload *ptxn)
{

}

// emulate memory I/O for application SW
void controller::reg_read(const uint32_t addr, uint32_t &data)
{

}

void controller::reg_write(const uint32_t addr, const uint32_t data)
{

}

void controller::burst_read(const uint32_t addr, unsigned char *data, const unsigned  int length)
{

}

void controller::burst_write(const uint32_t addr, const unsigned char *data, const unsigned  int length) 
{

}
