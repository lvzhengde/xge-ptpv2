#ifndef _SERVO_H__
#define _SERVO_H__

#include "datatypes.h"


class servo : public base_data
{
public:
  //member variables

  //member methods
  servo(ptpd *pApp);

  void reset_operator_messages(RunTimeOpts * rtOpts, PtpClock * ptpClock);
  
  void initClock(RunTimeOpts * rtOpts, PtpClock * ptpClock);
  
  void updateDelay(one_way_delay_filter * owd_filt, RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField);
  
  void updatePeerDelay(one_way_delay_filter * owd_filt, RunTimeOpts * rtOpts, 
      PtpClock * ptpClock, TimeInternal * correctionField, Boolean twoStep);
  
  void updateOffset(TimeInternal * send_time, TimeInternal * recv_time,
      offset_from_master_filter * ofm_filt, RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField);
  
  void servo_perform_clock_step(RunTimeOpts * rtOpts, PtpClock * ptpClock);
  
  void warn_operator_fast_slewing(RunTimeOpts * rtOpts, PtpClock * ptpClock, Integer32 adj);
  
  void warn_operator_slow_slewing(RunTimeOpts * rtOpts, PtpClock * ptpClock );
  
  void adjFreq_wrapper(RunTimeOpts * rtOpts, PtpClock * ptpClock, Integer32 adj);
  
  void updateClock(RunTimeOpts * rtOpts, PtpClock * ptpClock);

};

#endif // _SERVO_H__

