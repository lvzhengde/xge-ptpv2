#ifndef _SYS_H__
#define _SYS_H__

#include "datatypes.h"

#if defined(PTPD_LSBF)
  Integer16 flip16(Integer16 x);
  
  Integer32 flip32(Integer32 x);
#endif

class sys : public base_data 
{
public:
  //member variables

  //member methods
  sys(ptpd *pApp);

  char *dump_TimeInternal(const TimeInternal * p);
  
  char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2);
  
  int snprint_TimeInternal(char *s, int max_len, const TimeInternal * p);
  
  char *time2st(const TimeInternal * p);
  
  void DBG_time(const char *name, const TimeInternal  p);
  

  string translatePortState(PtpClock *ptpClock);
  
  int snprint_ClockIdentity(char *s, int max_len, const ClockIdentity id);
  
  int snprint_ClockIdentity_mac(char *s, int max_len, const ClockIdentity id);
  
  int ether_ntohost_cache(char *hostname, struct ether_addr *addr);
  
  int snprint_ClockIdentity_ntohost(char *s, int max_len, const ClockIdentity id);
  
  
  int snprint_PortIdentity(char *s, int max_len, const PortIdentity *id);
  
  void message(int priority, const char * format, ...);
  
  void increaseMaxDelayThreshold();
  
  void decreaseMaxDelayThreshold();
  
  void displayStats(RunTimeOpts * rtOpts, PtpClock * ptpClock);
  
  void recordSync(RunTimeOpts * rtOpts, UInteger16 sequenceId, TimeInternal * time);
  
  Boolean nanoSleep(TimeInternal * t);
  
  void getTime(TimeInternal * time);
  
  void setTime(TimeInternal * time);
  
  double getRand(void);
  
  #if !defined(__APPLE__)
  Boolean adjFreq(Integer32 adj);
  
  void setTimexFlags(int flags, Boolean quiet);
  
  void unsetTimexFlags(int flags, Boolean quiet);
  
  int getTimexFlags(void);
  
  Boolean checkTimexFlags(int flags);
  
  #if defined(MOD_TAI) &&  NTP_API == 4
  void setKernelUtcOffset(int utc_offset); 
  #endif /* MOD_TAI */
  
  #else
  
  void adjTime(Integer32 nanoseconds);
  
  #endif /* __APPLE__ */
  
  
  #if 0 && defined (linux)  /* NOTE: This is actually not used */
  
  long get_current_tickrate(void);
  
  #endif  /* defined(linux) */
};

#endif // _SYS_H__

