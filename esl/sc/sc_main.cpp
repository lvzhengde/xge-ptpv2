/*
 *  DESCRIPTION: Top level main for invoking SystemC model
 *  adapted from Verilator SystemC Example
*/

// SystemC global header
#include <systemc.h>

// Include common routines
#include <verilated.h>
// for waveform tracing, refer to CMakeLists.txt
// add "TRACE" option in verilate command
#if VM_TRACE
#include <verilated_vcd_sc.h>
#endif                                                                                                  

// Include testbench header
#include "testbench.h"

#define REPORT_DEFINE_GLOBALS
#include "reporting.h"  // reporting utilities

int sc_main(int argc, char* argv[]) 
{
  // Prevent unused variable warnings
  if (false && argc && argv) {}

  // Create logs/ directory in case we have traces to put under it
  Verilated::mkdir("logs");

  REPORT_ENABLE_ALL_REPORTING ();
  
  // Construct the testbench model
  unsigned int  sw_type = 0;
	testbench* pTb = new testbench("tb", sw_type);

  // do one evaluation before enabling waves, in order to allow
  // SystemC to interconnect everything for testing.
  sc_start(SC_ZERO_TIME);

#if VM_TRACE
  cout << "Enabling waves into logs/ptpv2_tlm.vcd...\n";
  Verilated::traceEverOn(true);
  VerilatedVcdSc* tfp = new VerilatedVcdSc;
  Vptp_top* pVtop = &pTb->pInstance->m_target_top.m_ptp_top;
  pVtop->trace(tfp, 99); // Trace 99 levels of hierarchy
  tfp->open("logs/ptpv2_tlm.vcd");
#endif

  //sc_start(); // Run until no more activity
  sc_start(1, SC_MS); 

  if (sc_get_status() == SC_PAUSED) {
      SC_REPORT_INFO("", "sc_stop called to terminate a paused simulation");
      sc_stop();
  }

#if VM_TRACE
  if (tfp) {
      tfp->close();
      tfp = nullptr;
  }
#endif  

  // Return good completion status
  return 0;
}
