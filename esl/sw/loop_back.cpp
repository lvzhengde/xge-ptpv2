#include "loop_back.h"
#include "controller.h"
#include "ptp_memmap.h"

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
  m_cpu_str = "Clock ID: " + to_string(m_pController->m_clock_id) + " Controller: " + to_string(m_pController->m_ID);
  cout << "\r\n  " << m_cpu_str << " Starting Loop Back Test!" << "\r\n";
  
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

  cout << "\r\n          "<< m_cpu_str  << "\r\n"
       << "=========================================================" << "\r\n" 
       << "            ####  Loop Back Test Complete!  #### " << "\r\n";
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
  data = 0x5; //adjust rtc and set timer interval to 7.8125ms //0x1;
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
  second = data;

  addr = base + CUR_TM_ADDR1;
  data = 0;
  REG_READ(addr, data);
  second = (second << 16) + ((data & 0xffff0000) >> 16);
  nanosecond = data & 0x0000ffff;

  addr = base + CUR_TM_ADDR2;
  data = 0;
  REG_READ(addr, data);
  nanosecond = (nanosecond << 16) + ((data & 0xffff0000) >> 16);
  
  //print uint64_t using format "%#llx"
  printf("Current RTC value second = %#llx, nanosecond = %#x \r\n", second, nanosecond);

  wait(100, SC_NS);

  //wait xms timer event
  wait(m_pController->m_ev_xms);
  cout << "\r\n" << "xms timer event received in loop back test!" << "\r\n";
}

//test frame tx/rx
void loop_back::frame_test()
{

}
