// ++
// DESCRIPTION: Top level main for invoking SystemC model
// adapted from Verilator SystemC Example
// --

// SystemC global header
#include <systemc.h>

// Include common routines
#include <verilated.h>

// Include testbench header
#include "testbench.h"

int sc_main(int argc, char* argv[]) 
{
  // Prevent unused variable warnings
  if (false && argc && argv) {}

  // Construct the testbench model
	testbench* pTb = new testbench("tb");

  //Initialize SC model
  //sc_start(1, SC_NS);
  //Run for exactly 200 ns
  //sc_start(200, SC_NS); 
	sc_start(SC_ZERO_TIME);
  sc_start(); // Run until no more activity

  if (sc_get_status() == SC_PAUSED) {
      SC_REPORT_INFO("", "sc_stop called to terminate a paused simulation");
      sc_stop();
  }

  // Final model cleanup
	pTb->clean();

  // Return good completion status
  return 0;
}
