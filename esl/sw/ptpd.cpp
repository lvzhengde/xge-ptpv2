#include "ptpd.h"
#include "controller.h"
#include "ptp_memmap.h"

//constructor
ptpd::ptpd(controller *pController)
  : MyApp(pController)
{
  //m_pController = pController;
}

//destructor
ptpd::~ptpd()
{

}

//initialize related variables
void ptpd::init()
{
  m_cpu_str = "Clock ID: " + to_string(m_pController->m_clock_id) + " Controller: " + to_string(m_pController->m_ID);
  
  cout << "\r\n            "<< m_cpu_str  << "\r\n"
       << "=========================================================" << "\r\n" 
       << "            ####  PTPv2 Protocol Test Start!  #### " << "\r\n" << "\r\n";

  wait(200, SC_NS);
}

//run loop back test
void ptpd::exec()
{
  init();

  exit();
}

//exit test and clean up
void ptpd::exit()
{
  wait(200, SC_NS);

  cout << "\r\n            "<< m_cpu_str  << "\r\n"
       << "=========================================================" << "\r\n" 
       << "            ####  PTPv2 Protocol Test Complete!  #### " << "\r\n";
}


