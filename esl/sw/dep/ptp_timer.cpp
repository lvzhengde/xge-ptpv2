/*-
 * Copyright (c) 2022-2023 Zhengde
 *
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments.
 * Copyright (c) 2009-2010 George V. Neville-Neil, 
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
 *
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   timer.c
 * @date   Wed Jun 23 09:41:26 2010
 * 
 * @brief  The timers which run the state machine.
 * 
 * Timers in the PTP daemon are run off of the signal system.  
 */

#include "common.h"

//ptp timer interrupt interval is 7.8125 ms
#define US_TIMER_INTERVAL (7812.5)


//constructor
ptp_timer::ptp_timer(ptpd *pApp)
{
    BASE_MEMBER_ASSIGN 

    operator_warned_interval_too_small = 0; 
}

//call this function in the interrupt service routine of intxms
//which is located in m_pController->isr_thread()
//in real application, it should be registered as a callback function
//for signal or interrupt
void 
ptp_timer::catch_alarm(int sig)
{
	elapsed++;
	/* be sure to NOT call DBG in asynchronous handlers! */
}

//do not depend on OS timer in fact
//relay on PTP HW timer interrupt
void 
ptp_timer::initTimer(void)
{
	DBG("initTimer\n");
	m_pController->ptr_ptp_timer = NULL;

    uint32_t addr = INT_BASE_ADDR + INT_MSK_OFT;
    uint32_t data = 0;

    REG_READ(addr, data);

    //disable intxms interrupt
    data &= 0xb;
	REG_WRITE(addr, data);

    //wait 1 us
	wait(1.0, SC_US);

    //clear intrrupt tick counter
	elapsed = 0;

    //enable intxms interrupt
	data |= 0x4;
	REG_WRITE(addr, data);

	//register this object in controller
	m_pController->ptr_ptp_timer = this;
}

void 
ptp_timer::timerUpdate(IntervalTimer * itimer)
{

	int i, delta;

	/*
	 * latch how many ticks we got since we were last called
	 * remember that catch_alarm() is totally asynchronous to this timerUpdate()
	 */
	delta = elapsed;
	elapsed = 0;

	if (delta <= 0)
		return;

	/*
	 * if time actually passed, then decrease every timer left
	 * the one(s) that went to zero or negative are:
	 *  a) rearmed at the original time (ignoring the time that may have passed ahead)
	 *  b) have their expiration latched until timerExpired() is called
	 */
	for (i = 0; i < TIMER_ARRAY_SIZE; ++i) {
		if ((itimer[i].interval) > 0 && ((itimer[i].left) -= delta) 
		    <= 0) {
			itimer[i].left = itimer[i].interval;
			itimer[i].expire = TRUE;
			DBG2("TimerUpdate:    Timer %u has now expired.   (Re-armed again with interval %d, left %d)\n", i, itimer[i].interval, itimer[i].left );
		}
	}

}

void 
ptp_timer::timerStop(UInteger16 index, IntervalTimer * itimer)
{
	if (index >= TIMER_ARRAY_SIZE)
		return;

	itimer[index].interval = 0;
	DBG2("timerStop:      Stopping timer %d.   (New interval: %d; New left: %d)\n", index, itimer[index].left , itimer[index].interval);
}

void 
ptp_timer::timerStart(UInteger16 index, float interval, IntervalTimer * itimer)
{
	if (index >= TIMER_ARRAY_SIZE)
		return;

	itimer[index].expire = FALSE;


	/*
	 *  US_TIMER_INTERVAL defines the minimum interval between sigalarms.
	 *  timerStart has a float parameter for the interval, which is casted to integer.
	 *  very small amounts are forced to expire ASAP by setting the interval to 1
	 */
	itimer[index].left = (int)((interval * 1E6) / US_TIMER_INTERVAL);
	if(itimer[index].left == 0){
		/*
		 * the interval is too small, raise it to 1 to make sure it expires ASAP
		 * Timer cancelation is done explicitelly with stopTimer()
		 */ 
		itimer[index].left = 1;

		if(!operator_warned_interval_too_small){
			operator_warned_interval_too_small = 1;
			/*
			 * using random uniform timers it is pratically guarantted that we hit the possible minimum timer.
			 * This is because of the current timer model based on periodic sigalarm, irrespective if the next
			 * event is close or far away in time.
			 *
			 * A solution would be to recode this whole module with a calendar queue, while keeping the same API:
			 * E.g.: http://www.isi.edu/nsnam/ns/doc/node35.html
			 *
			 * Having events that expire immediatly (ie, delayreq invocations using random timers) can lead to
			 * messages appearing in unexpected ordering, so the protocol implementation must check more conditions
			 * and not assume a certain ususal ordering
			 */
			DBG("Timer would be issued immediatly. Please raise dep/timer.c:US_TIMER_INTERVAL to hold %.2fs\n",
				interval
			);
			
		}
	}
	itimer[index].interval = itimer[index].left;

	DBG2("timerStart:     Set timer %d to %f.  New interval: %d; new left: %d\n", index, interval, itimer[index].left , itimer[index].interval);
}



/*
 * This function arms the timer with a uniform range, as requested by page 105 of the standard (for sending delayReqs.)
 * actual time will be U(0, interval * 2.0);
 *
 * PTPv1 algorithm was:
 *    ptpClock->R = getRand(&ptpClock->random_seed) % (PTP_DELAY_REQ_INTERVAL - 2) + 2;
 *    R is the number of Syncs to be received, before sending a new request
 * 
 */ 
void ptp_timer::timerStart_random(UInteger16 index, float interval, IntervalTimer * itimer)
{
	float new_value;

	new_value = m_pApp->m_ptr_sys->getRand() * interval * 2.0;
	DBG2(" timerStart_random: requested %.2f, got %.2f\n", interval, new_value);
	
	timerStart(index, new_value, itimer);
}



Boolean 
ptp_timer::timerExpired(UInteger16 index, IntervalTimer * itimer)
{
	timerUpdate(itimer);

	if (index >= TIMER_ARRAY_SIZE)
		return FALSE;

	if (!itimer[index].expire)
		return FALSE;

	itimer[index].expire = FALSE;


	DBG2("timerExpired:   Timer %d expired, taking actions.   current interval: %d; current left: %d\n", index, itimer[index].left , itimer[index].interval);

	return TRUE;
}
