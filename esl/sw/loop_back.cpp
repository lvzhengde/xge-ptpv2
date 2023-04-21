#include "loop_back.h"
#include "controller.h"
#include "ptp_memmap.h"
#include "reporting.h"               	 // reporting macros

using namespace std;

static const char *filename = "loop_back.cpp";  ///< filename for reporting

//constructor
loop_back::loop_back(controller *pController)
  : MyApp(pController)
{
  //m_pController = pController;
}

//destructor
loop_back::~loop_back()
{

}

//initialize related variables
void loop_back::init()
{
  std::ostringstream  msg;                      ///< log message
  
  msg.str ("");
  msg << "Clock ID: " << m_pController->m_clock_id
      << " Controller: " << m_pController->m_ID << " Starting Loop Back Test!";
  REPORT_INFO(filename, __FUNCTION__, msg.str());
  
  wait(200, SC_NS);
}

//run loop back test
void loop_back::exec()
{
  init();

  //test register access
  register_test();

  exit();
}

//exit test and clean up
void loop_back::exit()
{
  wait(200, SC_NS);

  std::ostringstream  msg;                      ///< log message

  msg.str ("");
  msg << "Clock ID: " << m_pController->m_clock_id
      << " Controller : " << m_pController->m_ID << endl 
      << "=========================================================" << endl 
      << "            ####  Loop Back Test Complete!  #### ";
  REPORT_INFO(filename, __FUNCTION__, msg.str());
}

//test register read/write
void loop_back::register_test()
{
  uint32_t base, addr, data;

  base = RTC_BLK_ADDR << 8;

  //set RTC
  cout << "Set initial RTC Value second: 0x1112345678, nanosecond: 0x1500000" << "\r\n";
  addr = base + SC_OFST_ADDR0;
  data = 0x11;
  REG_WRITE(addr, data);

  addr = base + SC_OFST_ADDR1;
  data = 0x12345678;
  REG_WRITE(addr, data);

  addr = base + NS_OFST_ADDR;
  data = 0x1500000;
  REG_WRITE(addr, data);

  addr = base + RTC_CTL_ADDR;
  data = 0x1;
  REG_WRITE(addr, data);
  
  //tick increment value
  addr = base + TICK_INC_ADDR;
  data = 0x1999999a;
  REG_WRITE(addr, data);
  printf("Write to address : %#x, value : %#x \r\n", addr, data);

  data = 0;
  REG_READ(addr, data);
  printf("Read from address : %#x, value : %#x \r\n", addr, data);

  //read current value from RTC
  cout << "Read current RTC Value frome register" << "\r\n";
  uint64_t second;
  uint32_t nanosecond;

  wait(121, SC_NS); //intentionally added

  addr = base + CUR_TM_ADDR0;
  data = 0;
  REG_READ(addr, data);
  printf("Read from address : %#x, value : %#x \r\n", addr, data);
  second = data;
  printf("second =  %#x \r\n", second);

  addr = base + CUR_TM_ADDR1;
  data = 0;
  REG_READ(addr, data);
  printf("Read from address : %#x, value : %#x \r\n", addr, data);
  second = (second << 16) + ((data & 0xffff0000) >> 16);
  printf("second =  %#x \r\n", second);
  nanosecond = data & 0x0000ffff;
  printf("nanosecond = %#x \r\n", nanosecond);

  addr = base + CUR_TM_ADDR2;
  data = 0;
  REG_READ(addr, data);
  printf("Read from address : %#x, value : %#x \r\n", addr, data);
  nanosecond = (nanosecond << 16) + ((data & 0xffff0000) >> 16);
  printf("nanosecond =  %#x  \r\n", nanosecond);

  printf("Current RTC value second = %#x, nanosecond = %#x \r\n", second, nanosecond);
}

//test frame tx/rx
void loop_back::frame_test()
{

}
