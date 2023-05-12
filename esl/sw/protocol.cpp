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
 * @file   protocol.c
 * @date   Wed Jun 23 09:40:39 2010
 * 
 * @brief  The code that handles the IEEE-1588 protocol and state machine
 * 
 * 
 */

#include "common.h"

//constructor
protocol::protocol(ptpd *pApp)
{
    BASE_MEMBER_ASSIGN
}

/* loop forever. doState() has a switch for the actions and events to be
   checked for 'port_state'. the actions and events may or may not change
   'port_state' by calling toState(), but once they are done we loop around
   again and perform the actions required for the new 'port_state'. */
void 
protocol::protocolExec(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBG("event POWERUP\n");

	toState(PTP_INITIALIZING, rtOpts, ptpClock);

	DBG("Debug Initializing...\n");

	for (;;)
	{
		/* 20110701: this main loop was rewritten to be more clear */

		if (ptpClock->portState == PTP_INITIALIZING) {
			if (!doInit(rtOpts, ptpClock)) {
				return;
			}
		} else {
			doState(rtOpts, ptpClock);
		}

		if (ptpClock->message_activity) {
			DBGV("activity\n");
		}

		/* Perform the heavy signal processing synchronously */
		m_pApp->m_ptr_startup->check_signals(rtOpts, ptpClock);
	}
}


/* perform actions required when leaving 'port_state' and entering 'state' */
void 
protocol::toState(UInteger8 state, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	ptpClock->message_activity = TRUE;
	
	/* leaving state tasks */
	switch (ptpClock->portState)
	{
	case PTP_MASTER:
		m_pApp->m_ptr_ptp_timer->timerStop(SYNC_INTERVAL_TIMER, ptpClock->itimer);  
		m_pApp->m_ptr_ptp_timer->timerStop(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer);
		m_pApp->m_ptr_ptp_timer->timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer); 
		break;
		
	case PTP_SLAVE:
		m_pApp->m_ptr_ptp_timer->timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
		
		if (ptpClock->delayMechanism == E2E)
			m_pApp->m_ptr_ptp_timer->timerStop(DELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		else if (ptpClock->delayMechanism == P2P)
			m_pApp->m_ptr_ptp_timer->timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		
		m_pApp->m_ptr_servo->initClock(rtOpts, ptpClock); 
		break;
		
	case PTP_PASSIVE:
		m_pApp->m_ptr_ptp_timer->timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		m_pApp->m_ptr_ptp_timer->timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
		break;
		
	case PTP_LISTENING:
		m_pApp->m_ptr_ptp_timer->timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
		break;
		
	default:
		break;
	}
	
	/* entering state tasks */

	/*
	 * No need of PRE_MASTER state because of only ordinary clock
	 * implementation.
	 */
	
	switch (state)
	{
	case PTP_INITIALIZING:
		DBG("state PTP_INITIALIZING\n");
		ptpClock->portState = PTP_INITIALIZING;
		break;
		
	case PTP_FAULTY:
		DBG("state PTP_FAULTY\n");
		ptpClock->portState = PTP_FAULTY;
		break;
		
	case PTP_DISABLED:
		DBG("state PTP_DISABLED\n");
		ptpClock->portState = PTP_DISABLED;
		break;
		
	case PTP_LISTENING:
		/* in Listening mode, make sure we don't send anything. Instead we just expect/wait for announces (started below) */
		m_pApp->m_ptr_ptp_timer->timerStop(SYNC_INTERVAL_TIMER,      ptpClock->itimer);
		m_pApp->m_ptr_ptp_timer->timerStop(ANNOUNCE_INTERVAL_TIMER,  ptpClock->itimer);
		m_pApp->m_ptr_ptp_timer->timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		m_pApp->m_ptr_ptp_timer->timerStop(DELAYREQ_INTERVAL_TIMER,  ptpClock->itimer);
		
		/*
		 *  Count how many _unique_ timeouts happen to us.
		 *  If we were already in Listen mode, then do not count this as a seperate reset, but stil do a new IGMP refresh
		 */
		if (ptpClock->portState != PTP_LISTENING) {
			ptpClock->reset_count++;
		}

		/* Revert to the original DelayReq interval, and ignore the one for the last master */
		ptpClock->logMinDelayReqInterval = rtOpts->initial_delayreq;

		/* force a IGMP refresh per reset */
		if (rtOpts->do_IGMP_refresh) {
			m_pApp->m_ptr_net->netRefreshIGMP(&ptpClock->netPath, rtOpts, ptpClock);
		}
		

		DBG("state PTP_LISTENING\n");
		INFO("  now in state PTP_LISTENING\n");
		m_pApp->m_ptr_ptp_timer->timerStart(ANNOUNCE_RECEIPT_TIMER, 
			   (ptpClock->announceReceiptTimeout) * 
			   (pow((float)2, (float)ptpClock->logAnnounceInterval)), 
			   ptpClock->itimer);
		ptpClock->portState = PTP_LISTENING;
		break;

	case PTP_MASTER:
		DBG("state PTP_MASTER\n");
		INFO("  now in state PTP_MASTER\n");
		
		m_pApp->m_ptr_ptp_timer->timerStart(SYNC_INTERVAL_TIMER, 
			   pow((float)2, (float)ptpClock->logSyncInterval), ptpClock->itimer);
		DBG("SYNC INTERVAL TIMER : %f \n",
		    pow((float)2, (float)ptpClock->logSyncInterval));
		m_pApp->m_ptr_ptp_timer->timerStart(ANNOUNCE_INTERVAL_TIMER, 
			   pow((float)2, (float)ptpClock->logAnnounceInterval), 
			   ptpClock->itimer);
		m_pApp->m_ptr_ptp_timer->timerStart(PDELAYREQ_INTERVAL_TIMER, 
			   pow((float)2, (float)ptpClock->logMinPdelayReqInterval), 
			   ptpClock->itimer);
		ptpClock->portState = PTP_MASTER;
		break;

	case PTP_PASSIVE:
		DBG("state PTP_PASSIVE\n");
		INFO("  now in state PTP_PASSIVE\n");

		
		m_pApp->m_ptr_ptp_timer->timerStart(PDELAYREQ_INTERVAL_TIMER, 
			   pow((float)2, (float)ptpClock->logMinPdelayReqInterval), 
			   ptpClock->itimer);
		m_pApp->m_ptr_ptp_timer->timerStart(ANNOUNCE_RECEIPT_TIMER, 
			   (ptpClock->announceReceiptTimeout) * 
			   (pow((float)2, (float)ptpClock->logAnnounceInterval)), 
			   ptpClock->itimer);
		ptpClock->portState = PTP_PASSIVE;
		m_pApp->m_ptr_bmc->p1(ptpClock, rtOpts);
		break;

	case PTP_UNCALIBRATED:
		DBG("state PTP_UNCALIBRATED\n");
		ptpClock->portState = PTP_UNCALIBRATED;
		break;

	case PTP_SLAVE:
		DBG("state PTP_SLAVE\n");
		INFO("  now in state PTP_SLAVE\n");
		
		m_pApp->m_ptr_servo->initClock(rtOpts, ptpClock);
		
		ptpClock->waitingForFollow = FALSE;
		ptpClock->waitingForDelayResp = FALSE;

		// FIXME: clear these vars inside initclock
		m_pApp->m_ptr_arith->clearTime(&ptpClock->delay_req_send_time);
		m_pApp->m_ptr_arith->clearTime(&ptpClock->delay_req_receive_time);
		m_pApp->m_ptr_arith->clearTime(&ptpClock->pdelay_req_send_time);
		m_pApp->m_ptr_arith->clearTime(&ptpClock->pdelay_req_receive_time);
		m_pApp->m_ptr_arith->clearTime(&ptpClock->pdelay_resp_send_time);
		m_pApp->m_ptr_arith->clearTime(&ptpClock->pdelay_resp_receive_time);
		
		m_pApp->m_ptr_ptp_timer->timerStart(OPERATOR_MESSAGES_TIMER,
			   OPERATOR_MESSAGES_INTERVAL,
			   ptpClock->itimer);
		
		m_pApp->m_ptr_ptp_timer->timerStart(ANNOUNCE_RECEIPT_TIMER,
			   (ptpClock->announceReceiptTimeout) * 
			   (pow((float)2, (float)ptpClock->logAnnounceInterval)), 
			   ptpClock->itimer);
		
		/*
		 * Previously, this state transition would start the
		 * delayreq timer immediately.  However, if this was
		 * faster than the first received sync, then the servo
		 * would drop the delayResp Now, we only start the
		 * timer after we receive the first sync (in
		 * handle_sync())
		 */
		ptpClock->waiting_for_first_sync = TRUE;
		ptpClock->waiting_for_first_delayresp = TRUE;

		ptpClock->portState = PTP_SLAVE;

#if !defined(__APPLE__)

		/* 
		 * leap second pending in kernel but no leap second 
		 * info from GM - withdraw kernel leap second
		 * if the flags have disappeared but we're past 
		 * leap second event, do nothing - kernel flags 
		 * will be unset in handleAnnounce()
		 */
		//if((!ptpClock->leap59 && !ptpClock->leap61) &&
		//    !ptpClock->leapSecondInProgress &&
		//   (checkTimexFlags(STA_INS) || checkTimexFlags(STA_DEL))) {
		//	WARNING("=== Leap second pending in kernel but not on "
		//		"GM: aborting kernel leap second\n");
		//	unsetTimexFlags(STA_INS | STA_DEL, TRUE);
		//}
#endif /* apple */
		break;
	default:
		DBG("to unrecognized state\n");
		break;
	}

	if (rtOpts->displayStats)
		m_pApp->m_ptr_sys->displayStats(rtOpts, ptpClock);
}


Boolean 
protocol::doInit(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBG("manufacturerIdentity: %s\n", MANUFACTURER_ID);
	DBG("manufacturerOUI: %02hhx:%02hhx:%02hhx \n",
		MANUFACTURER_ID_OUI0,
		MANUFACTURER_ID_OUI1,
		MANUFACTURER_ID_OUI2);
	/* initialize networking */
	m_pApp->m_ptr_net->netShutdown(&ptpClock->netPath);
	if (!m_pApp->m_ptr_net->netInit(&ptpClock->netPath, rtOpts, ptpClock)) {
		ERROR_("failed to initialize network\n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return FALSE;
	}
	
	/* initialize other stuff */
	m_pApp->m_ptr_bmc->initData(rtOpts, ptpClock);
	m_pApp->m_ptr_ptp_timer->initTimer();
	m_pApp->m_ptr_servo->initClock(rtOpts, ptpClock);
	m_pApp->m_ptr_bmc->m1(rtOpts, ptpClock );
	m_pApp->m_ptr_msg->msgPackHeader(ptpClock->msgObuf, ptpClock);
	
	toState(PTP_LISTENING, rtOpts, ptpClock);
	
	return TRUE;
}

/* handle actions and events for 'port_state' */
void 
protocol::doState(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	UInteger8 state;
	
	ptpClock->message_activity = FALSE;
	
	/* Process record_update (BMC algorithm) before everything else */
	switch (ptpClock->portState)
	{
	case PTP_LISTENING:
	case PTP_PASSIVE:
	case PTP_SLAVE:
	case PTP_MASTER:
		/*State decision Event*/

		/* If we received a valid Announce message, and can use it (record_update), then run the BMC algorithm */
		if(ptpClock->record_update)
		{
			DBG2("event STATE_DECISION_EVENT\n");
			ptpClock->record_update = FALSE;
			state = m_pApp->m_ptr_bmc->bmcExec(ptpClock->foreign, rtOpts, ptpClock); //bmc()
			if(state != ptpClock->portState)
				toState(state, rtOpts, ptpClock);
		}
		break;
		
	default:
		break;
	}
	
	
	switch (ptpClock->portState)
	{
	case PTP_FAULTY:
		/* imaginary troubleshooting */
		DBG("event FAULT_CLEARED\n");
		toState(PTP_INITIALIZING, rtOpts, ptpClock);
		return;
		
	case PTP_LISTENING:
	case PTP_UNCALIBRATED:
	case PTP_SLAVE:
	// passive mode behaves like the SLAVE state, in order to wait for the announce timeout of the current active master
	case PTP_PASSIVE:
		handle(rtOpts, ptpClock);
		
		/*
		 * handle SLAVE timers:
		 *   - No Announce message was received
		 *   - Time to send new delayReq  (miss of delayResp is not monitored explicitelly)
		 */
		if (m_pApp->m_ptr_ptp_timer->timerExpired(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer))
		{
			DBG("event ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES\n");
			ptpClock->number_foreign_records = 0;
			ptpClock->foreign_record_i = 0;

			if(!ptpClock->slaveOnly && 
			   ptpClock->clockQuality.clockClass != SLAVE_ONLY_CLOCK_CLASS) {
				m_pApp->m_ptr_bmc->m1(rtOpts,ptpClock);
				toState(PTP_MASTER, rtOpts, ptpClock);

			} else {
				/*
				 *  Force a reset when getting a timeout in state listening, that will lead to an IGMP reset
				 *  previously this was not the case when we were already in LISTENING mode
				 */
				toState(PTP_LISTENING, rtOpts, ptpClock);
			}
		}
		
		if (m_pApp->m_ptr_ptp_timer->timerExpired(OPERATOR_MESSAGES_TIMER, ptpClock->itimer)) {
			m_pApp->m_ptr_servo->reset_operator_messages(rtOpts, ptpClock);
		}


		if (ptpClock->delayMechanism == E2E) {
			if(m_pApp->m_ptr_ptp_timer->timerExpired(DELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				DBG2("event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				issueDelayReq(rtOpts,ptpClock);
			}
		} else if (ptpClock->delayMechanism == P2P) {
			if (m_pApp->m_ptr_ptp_timer->timerExpired(PDELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				issuePDelayReq(rtOpts,ptpClock);
			}

			/* FIXME: Path delay should also rearm its timer with the value received from the Master */
		}

                if (ptpClock->leap59 || ptpClock->leap61) {
                        DBGV("seconds to midnight: %.3f\n",m_pApp->m_ptr_arith->secondsToMidnight());
				}

                /* leap second period is over */
                 if(m_pApp->m_ptr_ptp_timer->timerExpired(LEAP_SECOND_PAUSE_TIMER,ptpClock->itimer) &&
                    ptpClock->leapSecondInProgress) {
                            /* 
                             * do not unpause offset calculation just
                             * yet, just indicate and it will be
                             * unpaused in handleAnnounce()
                            */
                            ptpClock->leapSecondPending = FALSE;
                            m_pApp->m_ptr_ptp_timer->timerStop(LEAP_SECOND_PAUSE_TIMER,ptpClock->itimer);
                    } 
		/* check if leap second is near and if we should pause updates */
		if( ptpClock->leapSecondPending &&
		    !ptpClock->leapSecondInProgress &&
		    (m_pApp->m_ptr_arith->secondsToMidnight() <= 
		    m_pApp->m_ptr_arith->getPauseAfterMidnight(ptpClock->logAnnounceInterval))) {
                            WARNING("=== Leap second event imminent - pausing "
				    "clock and offset updates\n");
                            ptpClock->leapSecondInProgress = TRUE;
#if !defined(__APPLE__)
      //                      if(!checkTimexFlags(ptpClock->leap61 ? 
						//STA_INS : STA_DEL)) {
      //                              WARNING("=== Kernel leap second flags have "
					 //   "been unset - attempting to set "
					 //   "again");
      //                              setTimexFlags(ptpClock->leap61 ? 
						//  STA_INS : STA_DEL, FALSE);
      //                      }
#endif /* apple */
			    /*
			     * start pause timer from now until [pause] after
			     * midnight, plus an extra second if inserting
			     * a leap second
			     */
			    m_pApp->m_ptr_ptp_timer->timerStart(LEAP_SECOND_PAUSE_TIMER,
				       m_pApp->m_ptr_arith->secondsToMidnight() + 
				       (int)ptpClock->leap61 + 
				       m_pApp->m_ptr_arith->getPauseAfterMidnight(ptpClock->logAnnounceInterval),
				       ptpClock->itimer);
		}

		break;

	case PTP_MASTER:
		/*
		 * handle SLAVE timers:
		 *   - Time to send new Sync
		 *   - Time to send new Announce
		 *   - Time to send new PathDelay
		 *      (DelayResp has no timer - as these are sent and retransmitted by the slaves)
		 */
	
		if (m_pApp->m_ptr_ptp_timer->timerExpired(SYNC_INTERVAL_TIMER, ptpClock->itimer)) {
			DBGV("event SYNC_INTERVAL_TIMEOUT_EXPIRES\n");
			issueSync(rtOpts, ptpClock);
		}
		
		if (m_pApp->m_ptr_ptp_timer->timerExpired(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer)) {
			DBGV("event ANNOUNCE_INTERVAL_TIMEOUT_EXPIRES\n");
			issueAnnounce(rtOpts, ptpClock);
		}
		
		if (ptpClock->delayMechanism == P2P) {
			if (m_pApp->m_ptr_ptp_timer->timerExpired(PDELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				issuePDelayReq(rtOpts,ptpClock);
			}
		}
		
		// TODO: why is handle() below expiretimer, while in slave is the opposite
		handle(rtOpts, ptpClock);
		
		if (ptpClock->slaveOnly || ptpClock->clockQuality.clockClass == SLAVE_ONLY_CLOCK_CLASS)
			toState(PTP_LISTENING, rtOpts, ptpClock);
		
		break;

	case PTP_DISABLED:
		handle(rtOpts, ptpClock);
		break;
		
	default:
		DBG("(doState) do unrecognized state\n");
		break;
	}
}


/* check and handle received messages */
void
protocol::handle(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	int ret;
	ssize_t length;
	Boolean isFromSelf;
	TimeInternal time = { 0, 0 };

	if (!ptpClock->message_activity) {
		ret = m_pApp->m_ptr_net->netSelect(0, &ptpClock->netPath);
		if (ret < 0) {
			PERROR("failed to poll sockets");
			toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		} else if (!ret) {
			/* DBGV("handle: nothing\n"); */
			return;
		}
		/* else length > 0 */
	}

	DBGV("handle: something\n");

	/* TODO: this should be based on the select actual FDs (if(FD_ISSET(...)) */
	length = m_pApp->m_ptr_net->netRecvEvent(ptpClock->msgIbuf, &time, &ptpClock->netPath);


	if (length < 0) {
		PERROR("failed to receive on the event socket");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	} else if (!length) {
		length = m_pApp->m_ptr_net->netRecvGeneral(ptpClock->msgIbuf, &time,
					&ptpClock->netPath);
		if (length < 0) {
			PERROR("failed to receive on the general socket");
			toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		} else if (!length)
			return;
	}

	/*
	 * make sure we use the TAI to UTC offset specified, if the master is sending the UTC_VALID bit
	 *
	 *
	 * On the slave, all timestamps that we handle here have been collected by our local clock (loopback+kernel-level timestamp)
	 * This includes delayReq just send, and delayResp, when it arrives.
	 *
	 * these are then adjusted to the same timebase of the Master (+34 leap seconds, as of 2011)
	 *
	 */
	DBGV("__UTC_offset: %d %d \n", ptpClock->currentUtcOffsetValid, ptpClock->currentUtcOffset);
	if (ptpClock->currentUtcOffsetValid) {
		time.seconds += ptpClock->currentUtcOffset;
	}

	ptpClock->message_activity = TRUE;

	if (length < HEADER_LENGTH) {
		ERROR_("message shorter than header length\n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	}

	m_pApp->m_ptr_msg->msgUnpackHeader(ptpClock->msgIbuf, &ptpClock->msgTmpHeader);

	if (ptpClock->msgTmpHeader.versionPTP != ptpClock->versionNumber) {
		DBG2("ignore version %d message\n", ptpClock->msgTmpHeader.versionPTP);
		return;
	}

	if(ptpClock->msgTmpHeader.domainNumber != ptpClock->domainNumber) {
		DBG2("ignore message from domainNumber %d\n", ptpClock->msgTmpHeader.domainNumber);
		return;
	}

	/*Spec 9.5.2.2*/	
	isFromSelf = (ptpClock->portIdentity.portNumber == ptpClock->msgTmpHeader.sourcePortIdentity.portNumber
		      && !memcmp(ptpClock->msgTmpHeader.sourcePortIdentity.clockIdentity, ptpClock->portIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH));

	/*
	 * subtract the inbound latency adjustment if it is not a loop
	 *  back and the time stamp seems reasonable 
	 */
	if (!isFromSelf && time.seconds > 0)
		m_pApp->m_ptr_arith->subTime(&time, &time, &rtOpts->inboundLatency);


#ifdef PTPD_DBG
	/* easy display of received messages */
	char *st;

	switch(ptpClock->msgTmpHeader.messageType)
	{
	case ANNOUNCE:
		st = "Announce";
		break;
	case SYNC:
		st = "Sync";
		break;
	case FOLLOW_UP:
		st = "FollowUp";
		break;
	case DELAY_REQ:
		st = "DelayReq";
		break;
	case DELAY_RESP:
		st = "DelayResp";
		break;
	case MANAGEMENT:
		st = "Management";
		break;
	default:
		st = "Unk";
		break;
	}
	DBG("      ==> %s received\n", st);
#endif

	/*
	 *  on the table below, note that only the event messsages are passed the local time,
	 *  (collected by us by loopback+kernel TS, and adjusted with UTC seconds
	 *
	 *  (SYNC / DELAY_REQ / PDELAY_REQ / PDELAY_RESP)
	 */
	switch(ptpClock->msgTmpHeader.messageType)
	{
	case ANNOUNCE:
		handleAnnounce(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
			       length, isFromSelf, rtOpts, ptpClock);
		break;
	case SYNC:
		handleSync(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
			   length, &time, isFromSelf, rtOpts, ptpClock);
		break;
	case FOLLOW_UP:
		handleFollowUp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
			       length, isFromSelf, rtOpts, ptpClock);
		break;
	case DELAY_REQ:
		handleDelayReq(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
			       length, &time, isFromSelf, rtOpts, ptpClock);
		break;
	case PDELAY_REQ:
		handlePDelayReq(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				length, &time, isFromSelf, rtOpts, ptpClock);
		break;  
	case DELAY_RESP:
		handleDelayResp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				length, isFromSelf, rtOpts, ptpClock);
		break;
	case PDELAY_RESP:
		handlePDelayResp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf,
				 &time, length, isFromSelf, rtOpts, ptpClock);
		break;
	case PDELAY_RESP_FOLLOW_UP:
		handlePDelayRespFollowUp(&ptpClock->msgTmpHeader, 
					 ptpClock->msgIbuf, length, 
					 isFromSelf, rtOpts, ptpClock);
		break;
	case MANAGEMENT:
		handleManagement(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				 length, isFromSelf, rtOpts, ptpClock);
		break;
	case SIGNALING:
		handleSignaling(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				length, isFromSelf, rtOpts, ptpClock);
		break;
	default:
		DBG("handle: unrecognized message\n");
		break;
	}

	if (rtOpts->displayPackets)
		m_pApp->m_ptr_msg->msgDump(ptpClock);

}

/*spec 9.5.3*/
void 
protocol::handleAnnounce(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
	       Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Boolean isFromCurrentParent = FALSE; 

	DBGV("HandleAnnounce : Announce message received : \n");

	if(length < ANNOUNCE_LENGTH) {
		ERROR_("short Announce message\n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	}

	//DBGV("  >> HandleAnnounce : %d  \n", ptpClock->portState);


	switch (ptpClock->portState) {
	case PTP_INITIALIZING:
	case PTP_FAULTY:
	case PTP_DISABLED:
		DBG("Handleannounce : disregard\n");
		return;
		
	case PTP_UNCALIBRATED:	
	case PTP_SLAVE:
		if (isFromSelf) {
			DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}
		
		/*
		 * Valid announce message is received : BMC algorithm
		 * will be executed 
		 */
		ptpClock->record_update = TRUE;

		isFromCurrentParent = !memcmp(
			ptpClock->parentPortIdentity.clockIdentity,
			header->sourcePortIdentity.clockIdentity,
			CLOCK_IDENTITY_LENGTH)	&& 
			(ptpClock->parentPortIdentity.portNumber == 
			 header->sourcePortIdentity.portNumber);
	
		switch (isFromCurrentParent) {
		case TRUE:
	   		m_pApp->m_ptr_msg->msgUnpackAnnounce(ptpClock->msgIbuf,
					  &ptpClock->msgTmp.announce);

			/* update datasets (file bmc.c) */
	   		m_pApp->m_ptr_bmc->s1(header,&ptpClock->msgTmp.announce,ptpClock, rtOpts);

			/* update current master in the fmr as well */
			memcpy(&ptpClock->foreign[ptpClock->foreign_record_best].header,
			       header,sizeof(MsgHeader));
			memcpy(&ptpClock->foreign[ptpClock->foreign_record_best].announce,
			       &ptpClock->msgTmp.announce,sizeof(MsgAnnounce));

			if(ptpClock->leapSecondInProgress) {
				/*
				 * if leap second period is over
				 * (pending == FALSE, inProgress ==
				 * TRUE), unpause offset calculation
				 * (received first announce after leap
				 * second)
				*/
				//if (!ptpClock->leapSecondPending) {
				//	WARNING("=== Leap second event over - "
				//		"resuming clock and offset updates\n");
				//	ptpClock->leapSecondInProgress=FALSE;
				//	ptpClock->leap59 = FALSE;
				//	ptpClock->leap61 = FALSE;
				//	unsetTimexFlags(STA_INS | STA_DEL, TRUE);
				//}
				;
			}
			DBG2("___ Announce: received Announce from current Master, so reset the Announce timer\n");
	   		/*Reset Timer handling Announce receipt timeout*/
	   		m_pApp->m_ptr_ptp_timer->timerStart(ANNOUNCE_RECEIPT_TIMER,
				   (ptpClock->announceReceiptTimeout) * 
				   (pow((float)2, (float)ptpClock->logAnnounceInterval)), 
				   ptpClock->itimer);

#ifdef PTP_EXPERIMENTAL
			// remember IP address of our master for -U option
			// todo: add this to bmc(), to cover the very first packet
			ptpClock->MasterAddr = ptpClock->netPath.lastRecvAddr;
#endif
			break;

		case FALSE:
			/*addForeign takes care of AnnounceUnpacking*/
			/* the actual decision to change masters is only done in  doState() / record_update == TRUE / bmc() */
			
			/* the original code always called: addforeign(new master) + timerstart(announce) */

			addForeign(ptpClock->msgIbuf,header,ptpClock);
			m_pApp->m_ptr_ptp_timer->timerStart(ANNOUNCE_RECEIPT_TIMER,
				   (ptpClock->announceReceiptTimeout) * 
				   (pow((float)2, (float)ptpClock->logAnnounceInterval)), 
				   ptpClock->itimer);
			break;

		default:
			DBG("HandleAnnounce : (isFromCurrentParent)"
			     "strange value ! \n");
	   		return;

		} /* switch on (isFromCurrentParrent) */
		break;

	/*
	 * Passive case: previously, this was handled in the default, just like the master case.
	 * This the announce would call addForeign(), but NOT reset the timer, so after 12s it would expire and we would come alive periodically 
	 *
	 * This code is now merged with the slave case to reset the timer, and call addForeign() if it's a third master
	 *
	 */
	case PTP_PASSIVE:
		if (isFromSelf) {
			DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}

		/*
		 * Valid announce message is received : BMC algorithm
		 * will be executed
		 */
		ptpClock->record_update = TRUE;

		isFromCurrentParent = !memcmp(
			ptpClock->parentPortIdentity.clockIdentity,
			header->sourcePortIdentity.clockIdentity,
			CLOCK_IDENTITY_LENGTH)	&&
			(ptpClock->parentPortIdentity.portNumber ==
			 header->sourcePortIdentity.portNumber);

		if (isFromCurrentParent) {
			m_pApp->m_ptr_msg->msgUnpackAnnounce(ptpClock->msgIbuf,
					  &ptpClock->msgTmp.announce);

			/* TODO: not in spec
			 * datasets should not be updated by another master
			 * this is the reason why we are PASSIVE and not SLAVE
			 * this should be p1(ptpClock, rtOpts);
			 */
			/* update datasets (file bmc.c) */
			m_pApp->m_ptr_bmc->s1(header,&ptpClock->msgTmp.announce,ptpClock, rtOpts);

			DBG("___ Announce: received Announce from current Master, so reset the Announce timer\n\n");
			/*Reset Timer handling Announce receipt timeout*/
			m_pApp->m_ptr_ptp_timer->timerStart(ANNOUNCE_RECEIPT_TIMER,
				   (ptpClock->announceReceiptTimeout) *
				   (pow((float)2, (float)ptpClock->logAnnounceInterval)),
				   ptpClock->itimer);
		} else {
			/*addForeign takes care of AnnounceUnpacking*/
			/* the actual decision to change masters is only done in  doState() / record_update == TRUE / bmc() */
			/* the original code always called: addforeign(new master) + timerstart(announce) */

			DBG("___ Announce: received Announce from another master, will add to the list, as it might be better\n\n");
			DBGV("this is to be decided immediatly by bmc())\n\n");
			addForeign(ptpClock->msgIbuf,header,ptpClock);
		}
		break;

		
	case PTP_MASTER:
	case PTP_LISTENING:           /* listening mode still causes timeouts in order to send IGMP refreshes */
	default :
		if (isFromSelf) {
			DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}

		DBGV("Announce message from another foreign master\n");
		addForeign(ptpClock->msgIbuf,header,ptpClock);
		ptpClock->record_update = TRUE;    /* run BMC() as soon as possible */
		break;

	} /* switch on (port_state) */
}


void 
protocol::handleSync(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
	   TimeInternal *time, Boolean isFromSelf, 
	   RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	TimeInternal OriginTimestamp;
	TimeInternal correctionField;

	Boolean isFromCurrentParent = FALSE;
	DBGV("Sync message received : \n");
	
	if (length < SYNC_LENGTH) {
		ERROR_("short Sync message\n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	}	

	switch (ptpClock->portState) {
	case PTP_INITIALIZING:
	case PTP_FAULTY:
	case PTP_DISABLED:
		DBGV("HandleSync : disregard\n");
		return;
		
	case PTP_UNCALIBRATED:	
	case PTP_SLAVE:
		if (isFromSelf) {
			DBGV("HandleSync: Ignore message from self \n");
			return;
		}
		isFromCurrentParent =
			!memcmp(ptpClock->parentPortIdentity.clockIdentity,
				header->sourcePortIdentity.clockIdentity,
				CLOCK_IDENTITY_LENGTH) && 
			(ptpClock->parentPortIdentity.portNumber == 
			 header->sourcePortIdentity.portNumber);
		
		if (isFromCurrentParent) {
			/* We only start our own delayReq timer after receiving the first sync */
			if (ptpClock->waiting_for_first_sync) {
				ptpClock->waiting_for_first_sync = FALSE;
				NOTICE("Received first Sync from Master\n");
				NOTICE("   going to arm DelayReq timer for the first time, with initial rate: %d\n",
					ptpClock->logMinDelayReqInterval
				);

				if (ptpClock->delayMechanism == E2E)
					m_pApp->m_ptr_ptp_timer->timerStart(DELAYREQ_INTERVAL_TIMER,
						   pow((float)2, (float)ptpClock->logMinDelayReqInterval),
						   ptpClock->itimer);
				else if (ptpClock->delayMechanism == P2P)
					m_pApp->m_ptr_ptp_timer->timerStart(PDELAYREQ_INTERVAL_TIMER,
						   pow((float)2, (float)ptpClock->logMinPdelayReqInterval),
						   ptpClock->itimer);
			}

			ptpClock->sync_receive_time.seconds = time->seconds;
			ptpClock->sync_receive_time.nanoseconds = time->nanoseconds;

			m_pApp->m_ptr_sys->recordSync(rtOpts, header->sequenceId, time);

			if ((header->flagField0 & PTP_TWO_STEP) == PTP_TWO_STEP) {
				DBG2("HandleSync: waiting for follow-up \n");

				ptpClock->waitingForFollow = TRUE;
				ptpClock->recvSyncSequenceId = 
					header->sequenceId;
				/*Save correctionField of Sync message*/
				m_pApp->m_ptr_arith->integer64_to_internalTime(
					header->correctionField,
					&correctionField);
				ptpClock->lastSyncCorrectionField.seconds = 
					correctionField.seconds;
				ptpClock->lastSyncCorrectionField.nanoseconds =
					correctionField.nanoseconds;
				break;
			} else {
				m_pApp->m_ptr_msg->msgUnpackSync(ptpClock->msgIbuf,
					      &ptpClock->msgTmp.sync);
				m_pApp->m_ptr_arith->integer64_to_internalTime(
					ptpClock->msgTmpHeader.correctionField,
					&correctionField);
				m_pApp->m_ptr_display->timeInternal_display(&correctionField);
				ptpClock->waitingForFollow = FALSE;
				m_pApp->m_ptr_arith->toInternalTime(&OriginTimestamp,
					       &ptpClock->msgTmp.sync.originTimestamp);
				m_pApp->m_ptr_servo->updateOffset(&OriginTimestamp,
					     &ptpClock->sync_receive_time,
					     &ptpClock->ofm_filt,rtOpts,
					     ptpClock,&correctionField);
				m_pApp->m_ptr_servo->updateClock(rtOpts,ptpClock);
				break;
			}
		} else {
			DBG("HandleSync: Sync message received from "
			     "another Master not our own \n");
		}
		break;

	case PTP_MASTER:
	default :
		if (!isFromSelf) {
			DBGV("HandleSync: Sync message received from "
			     "another Master  \n");
			break;
		} if (ptpClock->twoStepFlag) {
			DBGV("HandleSync: going to send followup message\n ");

			/*Add latency*/
			m_pApp->m_ptr_arith->addTime(time,time,&rtOpts->outboundLatency);
			issueFollowup(time,rtOpts,ptpClock);
			break;
		} else {
			DBGV("HandleSync: Sync message received from self\n ");
		}
	}
}


void 
protocol::handleFollowUp(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
	       Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBGV("Handlefollowup : Follow up message received \n");
	
	TimeInternal preciseOriginTimestamp;
	TimeInternal correctionField;
	Boolean isFromCurrentParent = FALSE;
	
	if (length < FOLLOW_UP_LENGTH)
	{
		ERROR_("short Follow up message\n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	}

	if (isFromSelf)
	{
		DBGV("Handlefollowup : Ignore message from self \n");
		return;
	}

	switch (ptpClock->portState)
	{
	case PTP_INITIALIZING:
	case PTP_FAULTY:
	case PTP_DISABLED:
	case PTP_LISTENING:
		DBGV("Handfollowup : disregard\n");
		return;
		
	case PTP_UNCALIBRATED:	
	case PTP_SLAVE:
		isFromCurrentParent = 
			!memcmp(ptpClock->parentPortIdentity.clockIdentity,
				header->sourcePortIdentity.clockIdentity,
				CLOCK_IDENTITY_LENGTH) && 
			(ptpClock->parentPortIdentity.portNumber == 
			 header->sourcePortIdentity.portNumber);
	 	
		if (isFromCurrentParent) {
			if (ptpClock->waitingForFollow)	{
				if ((ptpClock->recvSyncSequenceId == 
				     header->sequenceId)) {
					m_pApp->m_ptr_msg->msgUnpackFollowUp(ptpClock->msgIbuf,
							  &ptpClock->msgTmp.follow);
					ptpClock->waitingForFollow = FALSE;
					m_pApp->m_ptr_arith->toInternalTime(&preciseOriginTimestamp,
						       &ptpClock->msgTmp.follow.preciseOriginTimestamp);
					m_pApp->m_ptr_arith->integer64_to_internalTime(ptpClock->msgTmpHeader.correctionField,
								  &correctionField);
					m_pApp->m_ptr_arith->addTime(&correctionField,&correctionField,
						&ptpClock->lastSyncCorrectionField);

					/*
					send_time = preciseOriginTimestamp (received inside followup)
					recv_time = sync_receive_time (received as CMSG in handleEvent)
					*/
					m_pApp->m_ptr_servo->updateOffset(&preciseOriginTimestamp,
						     &ptpClock->sync_receive_time,&ptpClock->ofm_filt,
						     rtOpts,ptpClock,
						     &correctionField);
					m_pApp->m_ptr_servo->updateClock(rtOpts,ptpClock);
					break;
				} else
					INFO("Ignored followup, SequenceID doesn't match with "
					     "last Sync message \n");
			} else
				DBG2("Ignored followup, Slave was not waiting a follow up "
				     "message \n");
		} else
			DBG2("Ignored, Follow up message is not from current parent \n");

	case PTP_MASTER:
	case PTP_PASSIVE:
		DBGV("Ignored, Follow up message received from another master \n");
		break;

	default:
    		DBG("do unrecognized state1\n");
    		break;
	} /* Switch on (port_state) */

}


void
protocol::handleDelayReq(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
	       TimeInternal *time, Boolean isFromSelf,
	       RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	if (ptpClock->delayMechanism == E2E) {
		DBGV("delayReq message received : \n");
		
		if (length < DELAY_REQ_LENGTH) {
			ERROR_("short DelayReq message\n");
			toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}

		switch (ptpClock->portState) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
		case PTP_PASSIVE:
			DBGV("HandledelayReq : disregard\n");
			return;

		case PTP_SLAVE:
			if (isFromSelf)	{
				DBG("==> Handle DelayReq (%d)\n",
					 header->sequenceId);
				
				if ((ptpClock->sentDelayReqSequenceId - 1) != header->sequenceId) {
					INFO("HandledelayReq : disregard delayreq because of wrong SeqNo\n");
					break;
				}

				/*
				 * Get sending timestamp from IP stack
				 * with SO_TIMESTAMP
				 */

				/*
				 *  Make sure we process the REQ _before_ the RESP. While we could do this by any order,
				 *  (because it's implicitly indexed by (ptpClock->sentDelayReqSequenceId - 1), this is
				 *  now made explicit
				 */
				ptpClock->waitingForDelayResp = TRUE;

				ptpClock->delay_req_send_time.seconds = 
					time->seconds;
				ptpClock->delay_req_send_time.nanoseconds = 
					time->nanoseconds;

				/*Add latency*/
				m_pApp->m_ptr_arith->addTime(&ptpClock->delay_req_send_time,
					&ptpClock->delay_req_send_time,
					&rtOpts->outboundLatency);
				break;
			} else {
				DBG2("HandledelayReq : disreguard delayreq from other client\n");
			}
			break;

		case PTP_MASTER:
			m_pApp->m_ptr_msg->msgUnpackHeader(ptpClock->msgIbuf,
					&ptpClock->delayReqHeader);

#ifdef PTP_EXPERIMENTAL
			// remember IP address of this client for -U option
			ptpClock->LastSlaveAddr = ptpClock->netPath.lastRecvAddr;
#endif
					
			issueDelayResp(time,&ptpClock->delayReqHeader,
				       rtOpts,ptpClock);
			break;

		default:
			DBG("do unrecognized state2\n");
			break;
		}
	} else /* (Peer to Peer mode) */
		ERROR_("Delay messages are ignored in Peer to Peer mode\n");
}

void
protocol::handleDelayResp(MsgHeader *header, Octet *msgIbuf, ssize_t length,
		Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	if (ptpClock->delayMechanism == E2E) {
		Boolean isFromCurrentParent = FALSE;
		TimeInternal requestReceiptTimestamp;
		TimeInternal correctionField;

		DBGV("delayResp message received : \n");

		if(length < DELAY_RESP_LENGTH) {
			ERROR_("short DelayResp message\n");
			toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}

		switch(ptpClock->portState) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			DBGV("HandledelayResp : disregard\n");
			return;

		case PTP_SLAVE:
			m_pApp->m_ptr_msg->msgUnpackDelayResp(ptpClock->msgIbuf,
					   &ptpClock->msgTmp.resp);

			if ((memcmp(ptpClock->parentPortIdentity.clockIdentity,
				    header->sourcePortIdentity.clockIdentity,
				    CLOCK_IDENTITY_LENGTH) == 0 ) &&
			    (ptpClock->parentPortIdentity.portNumber == 
			     header->sourcePortIdentity.portNumber))
				isFromCurrentParent = TRUE;
			
			if ((memcmp(ptpClock->portIdentity.clockIdentity,
				    ptpClock->msgTmp.resp.requestingPortIdentity.clockIdentity,
				    CLOCK_IDENTITY_LENGTH) == 0) &&
			    ((ptpClock->sentDelayReqSequenceId - 1)== 
			     header->sequenceId) &&
			    (ptpClock->portIdentity.portNumber == 
			     ptpClock->msgTmp.resp.requestingPortIdentity.portNumber)
			    && isFromCurrentParent) {
				DBG("==> Handle DelayResp (%d)\n",
					 header->sequenceId);

				if (!ptpClock->waitingForDelayResp) {
					break;
				}

				ptpClock->waitingForDelayResp = FALSE;

				m_pApp->m_ptr_arith->toInternalTime(&requestReceiptTimestamp,
					       &ptpClock->msgTmp.resp.receiveTimestamp);
				ptpClock->delay_req_receive_time.seconds = 
					requestReceiptTimestamp.seconds;
				ptpClock->delay_req_receive_time.nanoseconds = 
					requestReceiptTimestamp.nanoseconds;

				m_pApp->m_ptr_arith->integer64_to_internalTime(
					header->correctionField,
					&correctionField);
				
				/*
					send_time = delay_req_send_time (received as CMSG in handleEvent)
					recv_time = requestReceiptTimestamp (received inside delayResp)
				*/

				m_pApp->m_ptr_servo->updateDelay(&ptpClock->owd_filt,
					    rtOpts,ptpClock, &correctionField);

				if (ptpClock->waiting_for_first_delayresp) {
					ptpClock->waiting_for_first_delayresp = FALSE;
					NOTICE("  received first DelayResp from Master\n");
				}

				if (rtOpts->ignore_delayreq_interval_master == 0) {
					DBGV("current delay_req: %d  new delay req: %d \n",
						ptpClock->logMinDelayReqInterval,
						header->logMessageInterval);

					/* Accept new DelayReq value from the Master */
					if (ptpClock->logMinDelayReqInterval != header->logMessageInterval) {
						NOTICE("  received new DelayReq frequency %d from Master (was: %d)\n",
							 header->logMessageInterval, ptpClock->logMinDelayReqInterval );
					}

					// collect new value indicated from the Master
					ptpClock->logMinDelayReqInterval = header->logMessageInterval;
					
					/* FIXME: the actual rearming of this timer with the new value only happens later in doState()/issueDelayReq() */
				} else {
					if (ptpClock->logMinDelayReqInterval != rtOpts->subsequent_delayreq) {
						NOTICE("  received new DelayReq frequency %d from command line (was: %d)\n",
							rtOpts->subsequent_delayreq, ptpClock->logMinDelayReqInterval);
					}
					ptpClock->logMinDelayReqInterval = rtOpts->subsequent_delayreq;
				}
			} else {
				DBG("HandledelayResp : delayResp doesn't match with the delayReq. \n");
				break;
			}
		}
	} else { /* (Peer to Peer mode) */
		ERROR_("Delay messages are disregarded in Peer to Peer mode \n");
	}

}


void 
protocol::handlePDelayReq(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
		TimeInternal *time, Boolean isFromSelf, 
		RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	if (ptpClock->delayMechanism == P2P) {
		DBGV("PdelayReq message received : \n");

		if(length < PDELAY_REQ_LENGTH) {
			ERROR_("short PDelayReq message\n");
			toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}

		switch (ptpClock->portState ) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			DBGV("HandlePdelayReq : disregard\n");
			return;

		case PTP_SLAVE:
		case PTP_MASTER:
		case PTP_PASSIVE:
			if (isFromSelf) {
				/*
				 * Get sending timestamp from IP stack
				 * with SO_TIMESTAMP
				 */
				ptpClock->pdelay_req_send_time.seconds = 
					time->seconds;
				ptpClock->pdelay_req_send_time.nanoseconds = 
					time->nanoseconds;
			
				/*Add latency*/
				m_pApp->m_ptr_arith->addTime(&ptpClock->pdelay_req_send_time,
					&ptpClock->pdelay_req_send_time,
					&rtOpts->outboundLatency);
				break;
			} else {
				m_pApp->m_ptr_msg->msgUnpackHeader(ptpClock->msgIbuf,
						&ptpClock->PdelayReqHeader);
				issuePDelayResp(time, header, rtOpts, 
						ptpClock);	
				break;
			}
		default:
			DBG("do unrecognized state3\n");
			break;
		}
	} else /* (End to End mode..) */
		ERROR_("Peer Delay messages are disregarded in End to End "
		      "mode \n");
}

void
protocol::handlePDelayResp(MsgHeader *header, Octet *msgIbuf, TimeInternal *time,
		 ssize_t length, Boolean isFromSelf, 
		 RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	if (ptpClock->delayMechanism == P2P) {
		/* Boolean isFromCurrentParent = FALSE; NOTE: This is never used in this function */
		TimeInternal requestReceiptTimestamp;
		TimeInternal correctionField;
	
		DBGV("PdelayResp message received : \n");

		if (length < PDELAY_RESP_LENGTH) {
			ERROR_("short PDelayResp message\n");
			toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}

		switch (ptpClock->portState ) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			DBGV("HandlePdelayResp : disregard\n");
			return;

		case PTP_SLAVE:
		case PTP_MASTER:
			if (ptpClock->twoStepFlag && isFromSelf) {
				m_pApp->m_ptr_arith->addTime(time,time,&rtOpts->outboundLatency);
				issuePDelayRespFollowUp(time,
							&ptpClock->PdelayReqHeader,
							rtOpts,ptpClock);
				break;
			}
			m_pApp->m_ptr_msg->msgUnpackPDelayResp(ptpClock->msgIbuf,
					    &ptpClock->msgTmp.presp);
		
#if 0  /* NOTE: This is never used in this function. Should it? */
			isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity,
						      header->sourcePortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH) && 
				(ptpClock->parentPortIdentity.portNumber == 
				 header->sourcePortIdentity.portNumber);
#endif	
			if (!((ptpClock->sentPDelayReqSequenceId == 
			       header->sequenceId) && 
			      (!memcmp(ptpClock->portIdentity.clockIdentity,ptpClock->msgTmp.presp.requestingPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH))
				 && ( ptpClock->portIdentity.portNumber == ptpClock->msgTmp.presp.requestingPortIdentity.portNumber)))	{

                                /* Two Step Clock */
				if ((header->flagField0 & PTP_TWO_STEP) == PTP_TWO_STEP) {
					/*Store t4 (Fig 35)*/
					ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
					ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;
					/*store t2 (Fig 35)*/
					m_pApp->m_ptr_arith->toInternalTime(&requestReceiptTimestamp,
						       &ptpClock->msgTmp.presp.requestReceiptTimestamp);
					ptpClock->pdelay_req_receive_time.seconds = requestReceiptTimestamp.seconds;
					ptpClock->pdelay_req_receive_time.nanoseconds = requestReceiptTimestamp.nanoseconds;
					
					m_pApp->m_ptr_arith->integer64_to_internalTime(header->correctionField,&correctionField);
					ptpClock->lastPdelayRespCorrectionField.seconds = correctionField.seconds;
					ptpClock->lastPdelayRespCorrectionField.nanoseconds = correctionField.nanoseconds;
				} else {
				/* One step Clock */
					/*Store t4 (Fig 35)*/
					ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
					ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;
					
					m_pApp->m_ptr_arith->integer64_to_internalTime(header->correctionField,&correctionField);
					m_pApp->m_ptr_servo->updatePeerDelay (&ptpClock->owd_filt,rtOpts,ptpClock,&correctionField,FALSE);
				}
				ptpClock->recvPDelayRespSequenceId = header->sequenceId;
				break;
			} else {
				DBGV("HandlePdelayResp : Pdelayresp doesn't "
				     "match with the PdelayReq. \n");
				break;
			}
			break; /* XXX added by gnn for safety */
		default:
			DBG("do unrecognized state4\n");
			break;
		}
	} else { /* (End to End mode..) */
		ERROR_("Peer Delay messages are disregarded in End to End "
		      "mode \n");
	}
}

void 
protocol::handlePDelayRespFollowUp(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
			 Boolean isFromSelf, RunTimeOpts *rtOpts, 
			 PtpClock *ptpClock){

	if (ptpClock->delayMechanism == P2P) {
		TimeInternal responseOriginTimestamp;
		TimeInternal correctionField;
	
		DBGV("PdelayRespfollowup message received : \n");
	
		if(length < PDELAY_RESP_FOLLOW_UP_LENGTH) {
			ERROR_("short PDelayRespfollowup message\n");
			toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}	
	
		switch(ptpClock->portState) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
			DBGV("HandlePdelayResp : disregard\n");
			return;
		
		case PTP_SLAVE:
		case PTP_MASTER:
			if ((header->sequenceId == 
			    ptpClock->sentPDelayReqSequenceId-1) && (header->sequenceId == ptpClock->recvPDelayRespSequenceId)) {
				m_pApp->m_ptr_msg->msgUnpackPDelayRespFollowUp(
					ptpClock->msgIbuf,
					&ptpClock->msgTmp.prespfollow);
				m_pApp->m_ptr_arith->toInternalTime(
					&responseOriginTimestamp,
					&ptpClock->msgTmp.prespfollow.responseOriginTimestamp);
				ptpClock->pdelay_resp_send_time.seconds = 
					responseOriginTimestamp.seconds;
				ptpClock->pdelay_resp_send_time.nanoseconds = 
					responseOriginTimestamp.nanoseconds;
				m_pApp->m_ptr_arith->integer64_to_internalTime(
					ptpClock->msgTmpHeader.correctionField,
					&correctionField);
				m_pApp->m_ptr_arith->addTime(&correctionField,&correctionField,
					&ptpClock->lastPdelayRespCorrectionField);
				m_pApp->m_ptr_servo->updatePeerDelay (&ptpClock->owd_filt,
						 rtOpts, ptpClock,
						 &correctionField,TRUE);
				break;
			}
		default:
			DBGV("Disregard PdelayRespFollowUp message  \n");
		}
	} else { /* (End to End mode..) */
		ERROR_("Peer Delay messages are disregarded in End to End "
		      "mode \n");
	}
}

void 
protocol::handleManagement(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
		 Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBGV("Management message received : \n");

	if (isFromSelf) {
		DBGV("handleManagement: Ignore message from self \n");
		return;
	}

	m_pApp->m_ptr_msg->msgUnpackManagement(ptpClock->msgIbuf,&ptpClock->msgTmp.manage, header, ptpClock);

	if(ptpClock->msgTmp.manage.tlv == NULL) {
		DBGV("handleManagement: TLV is empty\n");
		return;
	}

	/* is this an error status management TLV? */
	if(ptpClock->msgTmp.manage.tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS) {
		DBGV("handleManagement: Error Status TLV\n");
		m_pApp->m_ptr_msg->unpackMMErrorStatus(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		m_pApp->m_ptr_management->handleMMErrorStatus(&ptpClock->msgTmp.manage);
		/* cleanup msgTmp managementTLV */
		if(ptpClock->msgTmp.manage.tlv) {
			DBGV("cleanup ptpClock->msgTmp.manage message \n");
			if(ptpClock->msgTmp.manage.tlv->dataField) {
				m_pApp->m_ptr_msg->freeMMErrorStatusTLV(ptpClock->msgTmp.manage.tlv);
				free(ptpClock->msgTmp.manage.tlv->dataField);
			}
			free(ptpClock->msgTmp.manage.tlv);
		}
		return;
	} else if (ptpClock->msgTmp.manage.tlv->tlvType != TLV_MANAGEMENT) {
		/* do nothing, implemention specific handling */
		DBGV("handleManagement: Currently unsupported management TLV type\n");
		return;
	}

	switch(ptpClock->msgTmp.manage.tlv->managementId)
	{
	case MM_NULL_MANAGEMENT:
		DBGV("handleManagement: Null Management\n");
		m_pApp->m_ptr_management->handleMMNullManagement(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_CLOCK_DESCRIPTION:
		DBGV("handleManagement: Clock Description\n");
		m_pApp->m_ptr_msg->unpackMMClockDescription(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		m_pApp->m_ptr_management->handleMMClockDescription(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_USER_DESCRIPTION:
		DBGV("handleManagement: User Description\n");
		m_pApp->m_ptr_msg->unpackMMUserDescription(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		m_pApp->m_ptr_management->handleMMUserDescription(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_SAVE_IN_NON_VOLATILE_STORAGE:
		DBGV("handleManagement: Save In Non-Volatile Storage\n");
		m_pApp->m_ptr_management->handleMMSaveInNonVolatileStorage(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_RESET_NON_VOLATILE_STORAGE:
		DBGV("handleManagement: Reset Non-Volatile Storage\n");
		m_pApp->m_ptr_management->handleMMResetNonVolatileStorage(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_INITIALIZE:
		DBGV("handleManagement: Initialize\n");
		m_pApp->m_ptr_msg->unpackMMInitialize(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		m_pApp->m_ptr_management->handleMMInitialize(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_DEFAULT_DATA_SET:
		DBGV("handleManagement: Default Data Set\n");
		m_pApp->m_ptr_msg->unpackMMDefaultDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		m_pApp->m_ptr_management->handleMMDefaultDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_CURRENT_DATA_SET:
		DBGV("handleManagement: Current Data Set\n");
		m_pApp->m_ptr_msg->unpackMMCurrentDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		m_pApp->m_ptr_management->handleMMCurrentDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		break;
        case MM_PARENT_DATA_SET:
                DBGV("handleManagement: Parent Data Set\n");
                m_pApp->m_ptr_msg->unpackMMParentDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMParentDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_TIME_PROPERTIES_DATA_SET:
                DBGV("handleManagement: TimeProperties Data Set\n");
                m_pApp->m_ptr_msg->unpackMMTimePropertiesDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMTimePropertiesDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_PORT_DATA_SET:
                DBGV("handleManagement: Port Data Set\n");
                m_pApp->m_ptr_msg->unpackMMPortDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMPortDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_PRIORITY1:
                DBGV("handleManagement: Priority1\n");
                m_pApp->m_ptr_msg->unpackMMPriority1(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMPriority1(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_PRIORITY2:
                DBGV("handleManagement: Priority2\n");
                m_pApp->m_ptr_msg->unpackMMPriority2(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMPriority2(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_DOMAIN:
                DBGV("handleManagement: Domain\n");
                m_pApp->m_ptr_msg->unpackMMDomain(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMDomain(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
	case MM_SLAVE_ONLY:
		DBGV("handleManagement: Slave Only\n");
		m_pApp->m_ptr_msg->unpackMMSlaveOnly(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		m_pApp->m_ptr_management->handleMMSlaveOnly(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		break;
        case MM_LOG_ANNOUNCE_INTERVAL:
                DBGV("handleManagement: Log Announce Interval\n");
                m_pApp->m_ptr_msg->unpackMMLogAnnounceInterval(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMLogAnnounceInterval(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_ANNOUNCE_RECEIPT_TIMEOUT:
                DBGV("handleManagement: Announce Receipt Timeout\n");
                m_pApp->m_ptr_msg->unpackMMAnnounceReceiptTimeout(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMAnnounceReceiptTimeout(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_LOG_SYNC_INTERVAL:
                DBGV("handleManagement: Log Sync Interval\n");
                m_pApp->m_ptr_msg->unpackMMLogSyncInterval(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMLogSyncInterval(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_VERSION_NUMBER:
                DBGV("handleManagement: Version Number\n");
                m_pApp->m_ptr_msg->unpackMMVersionNumber(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMVersionNumber(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_ENABLE_PORT:
                DBGV("handleManagement: Enable Port\n");
                m_pApp->m_ptr_management->handleMMEnablePort(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_DISABLE_PORT:
                DBGV("handleManagement: Disable Port\n");
                m_pApp->m_ptr_management->handleMMDisablePort(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_TIME:
                DBGV("handleManagement: Time\n");
                m_pApp->m_ptr_msg->unpackMMTime(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMTime(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_CLOCK_ACCURACY:
                DBGV("handleManagement: Clock Accuracy\n");
                m_pApp->m_ptr_msg->unpackMMClockAccuracy(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMClockAccuracy(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_UTC_PROPERTIES:
                DBGV("handleManagement: Utc Properties\n");
                m_pApp->m_ptr_msg->unpackMMUtcProperties(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMUtcProperties(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_TRACEABILITY_PROPERTIES:
                DBGV("handleManagement: Traceability Properties\n");
                m_pApp->m_ptr_msg->unpackMMTraceabilityProperties(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMTraceabilityProperties(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_DELAY_MECHANISM:
                DBGV("handleManagement: Delay Mechanism\n");
                m_pApp->m_ptr_msg->unpackMMDelayMechanism(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMDelayMechanism(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
                DBGV("handleManagement: Log Min Pdelay Req Interval\n");
                m_pApp->m_ptr_msg->unpackMMLogMinPdelayReqInterval(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                m_pApp->m_ptr_management->handleMMLogMinPdelayReqInterval(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
                break;
	case MM_FAULT_LOG:
	case MM_FAULT_LOG_RESET:
	case MM_TIMESCALE_PROPERTIES:
	case MM_UNICAST_NEGOTIATION_ENABLE:
	case MM_PATH_TRACE_LIST:
	case MM_PATH_TRACE_ENABLE:
	case MM_GRANDMASTER_CLUSTER_TABLE:
	case MM_UNICAST_MASTER_TABLE:
	case MM_UNICAST_MASTER_MAX_TABLE_SIZE:
	case MM_ACCEPTABLE_MASTER_TABLE:
	case MM_ACCEPTABLE_MASTER_TABLE_ENABLED:
	case MM_ACCEPTABLE_MASTER_MAX_TABLE_SIZE:
	case MM_ALTERNATE_MASTER:
	case MM_ALTERNATE_TIME_OFFSET_ENABLE:
	case MM_ALTERNATE_TIME_OFFSET_NAME:
	case MM_ALTERNATE_TIME_OFFSET_MAX_KEY:
	case MM_ALTERNATE_TIME_OFFSET_PROPERTIES:
	case MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
	case MM_TRANSPARENT_CLOCK_PORT_DATA_SET:
	case MM_PRIMARY_DOMAIN:
		DBGV("handleManagement: Currently unsupported managementTLV %d\n",
				ptpClock->msgTmp.manage.tlv->managementId);
		m_pApp->m_ptr_management->handleErrorManagementMessage(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp,
			ptpClock, ptpClock->msgTmp.manage.tlv->managementId,
			NOT_SUPPORTED);
		break;
	default:
		DBGV("handleManagement: Unknown managementTLV %d\n",
				ptpClock->msgTmp.manage.tlv->managementId);
		m_pApp->m_ptr_management->handleErrorManagementMessage(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp,
			ptpClock, ptpClock->msgTmp.manage.tlv->managementId,
			NO_SUCH_ID);

	}

	/* send management message response or acknowledge */
	if(ptpClock->outgoingManageTmp.tlv->tlvType == TLV_MANAGEMENT) {
		if(ptpClock->outgoingManageTmp.actionField == RESPONSE ||
				ptpClock->outgoingManageTmp.actionField == ACKNOWLEDGE) {
			issueManagementRespOrAck(&ptpClock->outgoingManageTmp, rtOpts, ptpClock);
		}
	} else if(ptpClock->outgoingManageTmp.tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS) {
		issueManagementErrorStatus(&ptpClock->outgoingManageTmp, rtOpts, ptpClock);
	}

	/* cleanup msgTmp managementTLV */
	m_pApp->m_ptr_msg->freeManagementTLV(&ptpClock->msgTmp.manage);
	/* cleanup outgoing managementTLV */
	m_pApp->m_ptr_msg->freeManagementTLV(&ptpClock->outgoingManageTmp);

}

void 
protocol::handleSignaling(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
		     Boolean isFromSelf, RunTimeOpts *rtOpts, 
		     PtpClock *ptpClock)
{}


/*Pack and send on general multicast ip adress an Announce message*/
void 
protocol::issueAnnounce(RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	m_pApp->m_ptr_msg->msgPackAnnounce(ptpClock->msgObuf,ptpClock);

	if (!m_pApp->m_ptr_net->netSendGeneral(ptpClock->msgObuf,ANNOUNCE_LENGTH,
			    &ptpClock->netPath, 0)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		DBGV("Announce message can't be sent -> FAULTY state \n");
	} else {
		DBGV("Announce MSG sent ! \n");
		ptpClock->sentAnnounceSequenceId++;
	}
}



/*Pack and send on event multicast ip adress a Sync message*/
void
protocol::issueSync(RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;
	m_pApp->m_ptr_sys->getTime(&internalTime);
	m_pApp->m_ptr_arith->fromInternalTime(&internalTime,&originTimestamp);

	m_pApp->m_ptr_msg->msgPackSync(ptpClock->msgObuf,&originTimestamp,ptpClock);

	if (!m_pApp->m_ptr_net->netSendEvent(ptpClock->msgObuf,SYNC_LENGTH,&ptpClock->netPath, 0)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		DBGV("Sync message can't be sent -> FAULTY state \n");
	} else {
		DBGV("Sync MSG sent ! \n");
		ptpClock->sentSyncSequenceId++;
	}
}


/*Pack and send on general multicast ip adress a FollowUp message*/
void
protocol::issueFollowup(TimeInternal *time,RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Timestamp preciseOriginTimestamp;
	m_pApp->m_ptr_arith->fromInternalTime(time,&preciseOriginTimestamp);
	
	m_pApp->m_ptr_msg->msgPackFollowUp(ptpClock->msgObuf,&preciseOriginTimestamp,ptpClock);
	
	if (!m_pApp->m_ptr_net->netSendGeneral(ptpClock->msgObuf,FOLLOW_UP_LENGTH,
			    &ptpClock->netPath, 0)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		DBGV("FollowUp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("FollowUp MSG sent ! \n");
	}
}


/*Pack and send on event multicast ip adress a DelayReq message*/
void
protocol::issueDelayReq(RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;

	DBG("==> Issue DelayReq (%d)\n", ptpClock->sentDelayReqSequenceId );

	/* call GTOD. This time is later replaced on handle_delayreq, to get the actual send timestamp from the OS */
	m_pApp->m_ptr_sys->getTime(&internalTime);
	m_pApp->m_ptr_arith->fromInternalTime(&internalTime,&originTimestamp);

	// uses current sentDelayReqSequenceId
	m_pApp->m_ptr_msg->msgPackDelayReq(ptpClock->msgObuf,&originTimestamp,ptpClock);

	Integer32 dst = 0;
#ifdef PTP_EXPERIMENTAL
	if (rtOpts->do_hybrid_mode) {
		dst = ptpClock->MasterAddr;
	}
#endif

	if (!m_pApp->m_ptr_net->netSendEvent(ptpClock->msgObuf,DELAY_REQ_LENGTH,
			  &ptpClock->netPath, dst)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		DBGV("delayReq message can't be sent -> FAULTY state \n");
	} else {
		DBGV("DelayReq MSG sent ! \n");
		ptpClock->sentDelayReqSequenceId++;

		/* From now on, we will only accept delayreq and delayresp of (sentDelayReqSequenceId - 1) */

		/* Explicitelly re-arm timer for sending the next delayReq */
		m_pApp->m_ptr_ptp_timer->timerStart_random(DELAYREQ_INTERVAL_TIMER,
		   pow((float)2, (float)ptpClock->logMinDelayReqInterval),
		   ptpClock->itimer);
	}
}

/*Pack and send on event multicast ip adress a PDelayReq message*/
void
protocol::issuePDelayReq(RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;
	m_pApp->m_ptr_sys->getTime(&internalTime);
	m_pApp->m_ptr_arith->fromInternalTime(&internalTime,&originTimestamp);
	
	m_pApp->m_ptr_msg->msgPackPDelayReq(ptpClock->msgObuf,&originTimestamp,ptpClock);
	if (!m_pApp->m_ptr_net->netSendPeerEvent(ptpClock->msgObuf,PDELAY_REQ_LENGTH,
			      &ptpClock->netPath)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		DBGV("PdelayReq message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PDelayReq MSG sent ! \n");
		ptpClock->sentPDelayReqSequenceId++;
	}
}

/*Pack and send on event multicast ip adress a PDelayResp message*/
void
protocol::issuePDelayResp(TimeInternal *time,MsgHeader *header,RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{
	Timestamp requestReceiptTimestamp;
	m_pApp->m_ptr_arith->fromInternalTime(time,&requestReceiptTimestamp);
	m_pApp->m_ptr_msg->msgPackPDelayResp(ptpClock->msgObuf,header,
			  &requestReceiptTimestamp,ptpClock);

	if (!m_pApp->m_ptr_net->netSendPeerEvent(ptpClock->msgObuf,PDELAY_RESP_LENGTH,
			      &ptpClock->netPath)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		DBGV("PdelayResp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PDelayResp MSG sent ! \n");
	}
}


/*Pack and send on event multicast ip adress a DelayResp message*/
void
protocol::issueDelayResp(TimeInternal *time,MsgHeader *header,RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Timestamp requestReceiptTimestamp;
	m_pApp->m_ptr_arith->fromInternalTime(time,&requestReceiptTimestamp);
	m_pApp->m_ptr_msg->msgPackDelayResp(ptpClock->msgObuf,header,&requestReceiptTimestamp,
			 ptpClock);

	Integer32 dst = 0;
#ifdef PTP_EXPERIMENTAL
	if (rtOpts->do_hybrid_mode) {
		dst = ptpClock->LastSlaveAddr;
	}
#endif

	if (!m_pApp->m_ptr_net->netSendGeneral(ptpClock->msgObuf,PDELAY_RESP_LENGTH,
			    &ptpClock->netPath, dst)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		DBGV("delayResp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PDelayResp MSG sent ! \n");
	}
}


void
protocol::issuePDelayRespFollowUp(TimeInternal *time, MsgHeader *header,
			     RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Timestamp responseOriginTimestamp;
	m_pApp->m_ptr_arith->fromInternalTime(time,&responseOriginTimestamp);

	m_pApp->m_ptr_msg->msgPackPDelayRespFollowUp(ptpClock->msgObuf,header,
				  &responseOriginTimestamp,ptpClock);
	if (!m_pApp->m_ptr_net->netSendPeerGeneral(ptpClock->msgObuf,
				PDELAY_RESP_FOLLOW_UP_LENGTH,
				&ptpClock->netPath)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		DBGV("PdelayRespFollowUp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PDelayRespFollowUp MSG sent ! \n");
	}
}

void 
protocol::issueManagement(MsgHeader *header,MsgManagement *manage,RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{}

void 
protocol::issueManagementRespOrAck(MsgManagement *outgoing, RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{
	/* pack ManagementTLV */
	m_pApp->m_ptr_msg->msgPackManagementTLV( ptpClock->msgObuf, outgoing, ptpClock);

	/* set header messageLength, the outgoing->tlv->lengthField is now valid */
	outgoing->header.messageLength = MANAGEMENT_LENGTH +
					TLV_LENGTH +
					outgoing->tlv->lengthField;

	m_pApp->m_ptr_msg->msgPackManagement( ptpClock->msgObuf, outgoing, ptpClock);

	if(!m_pApp->m_ptr_net->netSendGeneral(ptpClock->msgObuf, outgoing->header.messageLength,
				&ptpClock->netPath, 0)) {
		DBGV("Management response/acknowledge can't be sent -> FAULTY state \n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
	} else {
		DBGV("Management response/acknowledge msg sent \n");
	}
}

void
protocol::issueManagementErrorStatus(MsgManagement *outgoing, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	/* pack ManagementErrorStatusTLV */
	m_pApp->m_ptr_msg->msgPackManagementErrorStatusTLV( ptpClock->msgObuf, outgoing, ptpClock);

	/* set header messageLength, the outgoing->tlv->lengthField is now valid */
	outgoing->header.messageLength = MANAGEMENT_LENGTH +
					TLV_LENGTH +
					outgoing->tlv->lengthField;

	m_pApp->m_ptr_msg->msgPackManagement( ptpClock->msgObuf, outgoing, ptpClock);

	if(!m_pApp->m_ptr_net->netSendGeneral(ptpClock->msgObuf, outgoing->header.messageLength,
				&ptpClock->netPath, 0)) {
		DBGV("Management error status can't be sent -> FAULTY state \n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
	} else {
		DBGV("Management error status msg sent \n");
	}

}

void
protocol::addForeign(Octet *buf,MsgHeader *header,PtpClock *ptpClock)
{
	int i,j;
	Boolean found = FALSE;

	j = ptpClock->foreign_record_best;
	
	/*Check if Foreign master is already known*/
	for (i=0;i<ptpClock->number_foreign_records;i++) {
		if (!memcmp(header->sourcePortIdentity.clockIdentity,
			    ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity,
			    CLOCK_IDENTITY_LENGTH) && 
		    (header->sourcePortIdentity.portNumber == 
		     ptpClock->foreign[j].foreignMasterPortIdentity.portNumber))
		{
			/*Foreign Master is already in Foreignmaster data set*/
			ptpClock->foreign[j].foreignMasterAnnounceMessages++; 
			found = TRUE;
			DBGV("addForeign : AnnounceMessage incremented \n");
			m_pApp->m_ptr_msg->msgUnpackHeader(buf,&ptpClock->foreign[j].header);
			m_pApp->m_ptr_msg->msgUnpackAnnounce(buf,&ptpClock->foreign[j].announce);
			break;
		}
	
		j = (j+1)%ptpClock->number_foreign_records;
	}

	/*New Foreign Master*/
	if (!found) {
		if (ptpClock->number_foreign_records < 
		    ptpClock->max_foreign_records) {
			ptpClock->number_foreign_records++;
		}
		j = ptpClock->foreign_record_i;
		
		/*Copy new foreign master data set from Announce message*/
		m_pApp->m_ptr_msg->copyClockIdentity(ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity,
		       header->sourcePortIdentity.clockIdentity);
		ptpClock->foreign[j].foreignMasterPortIdentity.portNumber = 
			header->sourcePortIdentity.portNumber;
		ptpClock->foreign[j].foreignMasterAnnounceMessages = 0;
		
		/*
		 * header and announce field of each Foreign Master are
		 * usefull to run Best Master Clock Algorithm
		 */
		m_pApp->m_ptr_msg->msgUnpackHeader(buf,&ptpClock->foreign[j].header);
		m_pApp->m_ptr_msg->msgUnpackAnnounce(buf,&ptpClock->foreign[j].announce);
		DBGV("New foreign Master added \n");
		
		ptpClock->foreign_record_i = 
			(ptpClock->foreign_record_i+1) % 
			ptpClock->max_foreign_records;	
	}
}
