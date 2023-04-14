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

#define REPORT_DEFINE_GLOBALS
#include "reporting.h"  // reporting utilities

int sc_main(int argc, char* argv[]) 
{
  // Prevent unused variable warnings
  if (false && argc && argv) {}

  REPORT_ENABLE_ALL_REPORTING ();
  
  //set time resolution and time unit
  //sc_set_time_resolution(100, SC_PS); // deprecated function but still useful, default is 1 PS
  //sc_set_default_time_unit(1, SC_NS); // change time unit to 1 nanosecond

  // Construct the testbench model
  unsigned int  sw_type = 0;
	testbench* pTb = new testbench("tb", sw_type);

  //Initialize SC model
  //sc_start(1, SC_NS);
  //Run for exactly 200 ns
  //sc_start(200, SC_NS); 
	sc_start(SC_ZERO_TIME);
  //sc_start(); // Run until no more activity
  sc_start(1, SC_MS); 

  if (sc_get_status() == SC_PAUSED) {
      SC_REPORT_INFO("", "sc_stop called to terminate a paused simulation");
      sc_stop();
  }

  // Return good completion status
  return 0;
}
