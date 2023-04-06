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
  m_data_ptr = new unsigned char [512];
  m_ptxn->set_data_ptr(m_data_ptr);
}

/// Destructor
controller::~controller()
{
  delete [] m_data_ptr;
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
  std::ostringstream  msg;                      ///< log message
  tlm::tlm_generic_payload  *transaction_ptr;  

  msg.str ("");
  msg << "Controller: " << m_ID << " Starting Bus Traffic";
  REPORT_INFO(filename, __FUNCTION__, msg.str());

  // send I/O access request
  request_out_port->write(ptxn);

  // get response
  response_in_port->read(transaction_ptr);

  // check validation
  if ((transaction_ptr ->get_response_status() != tlm::TLM_OK_RESPONSE)
      || (transaction_ptr ->get_command() != ptxn->get_command())
      || (transaction_ptr ->get_address() != ptxn->get_address()))
  {
    msg.str ("");
    msg << m_ID << "Transaction ERROR";
    REPORT_FATAL(filename, __FUNCTION__, msg.str());   
  }
}

// emulate memory I/O for application SW
void controller::reg_read(const uint32_t addr, uint32_t &data)
{
  //lock shared resources(gp and fifo)
  m_bus_mutex.lock();

  //set transaction
  m_ptxn->set_command          ( tlm::TLM_READ_COMMAND        );
  m_ptxn->set_address          ( addr                  );
  m_ptxn->set_data_length      ( 4              );
  m_ptxn->set_streaming_width  ( 4              );
  m_ptxn->set_response_status  ( tlm::TLM_INCOMPLETE_RESPONSE );

  //access bus through sc_fifo
  transaction_manip(m_ptxn);

  //convert data
  data = (m_data_ptr[3]<<24) | (m_data_ptr[2]<<16) | (m_data_ptr[1]<<8) | m_data_ptr[0];

  //unlock shared resources
  m_bus_mutex.unlock();

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
