#ifndef _PTP_TIMER_H__
#define _PTP_TIMER_H__

#include "datatypes.h"

#ifdef WIN_PTP
void  CALLBACK TimeEvent(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
 
void StartEventTime(DWORD_PTR duser);
#endif

void catch_alarm(int sig);

void initTimer(void);

void timerUpdate(IntervalTimer * itimer);

void timerStop(UInteger16 index, IntervalTimer * itimer);

void timerStart(UInteger16 index, float interval, IntervalTimer * itimer);

void timerStart_random(UInteger16 index, float interval, IntervalTimer * itimer);

Boolean timerExpired(UInteger16 index, IntervalTimer * itimer);

#endif // _PTP_TIMER_H__


