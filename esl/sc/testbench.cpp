#include "testbench.h"

void testbench::say_hello()
{
  cout << "It's in SystemC testbench!" << endl;
	wait(10, SC_NS);
	cout << "10ns elapsed!" <<endl;
	wait(10, SC_NS);
	cout << "20ns elapsed!" <<endl;
	wait(10, SC_NS);
	cout << "30ns elapsed!" <<endl;

	sc_stop();
}

void testbench::clean()
{
    pTop->final();
    pChannel->final();
}

