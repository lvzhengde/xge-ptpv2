#include "common.h"

//global variables
RunTimeOpts rtOpts;			//configuration data     

PtpClock *G_ptpClock;

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

	PtpClock *ptpClock;
	Integer16 ret;

	/* initialize run-time options to default values */
	rtOpts.announceInterval = DEFAULT_ANNOUNCE_INTERVAL;
	rtOpts.syncInterval = DEFAULT_SYNC_INTERVAL;
	rtOpts.clockQuality.clockAccuracy = DEFAULT_CLOCK_ACCURACY;
	rtOpts.clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
	rtOpts.clockQuality.offsetScaledLogVariance = DEFAULT_CLOCK_VARIANCE;
	rtOpts.priority1 = DEFAULT_PRIORITY1;
	rtOpts.priority2 = DEFAULT_PRIORITY2;
	rtOpts.domainNumber = DEFAULT_DOMAIN_NUMBER;
#ifdef PTP_EXPERIMENTAL
	rtOpts.mcast_group_Number = 0;
	rtOpts.do_hybrid_mode = 0;
#endif
	
	// rtOpts.slaveOnly = FALSE;
	rtOpts.currentUtcOffset = DEFAULT_UTC_OFFSET;
	rtOpts.ifaceName[0] = '\0';
	rtOpts.do_unicast_mode = 0;

	rtOpts.noAdjust = NO_ADJUST;  // false
	// rtOpts.displayStats = FALSE;
	/* Deep display of all packets seen by the daemon */
	rtOpts.displayPackets = FALSE;
	// rtOpts.unicastAddress
	rtOpts.ap = DEFAULT_AP;
	rtOpts.ai = DEFAULT_AI;
	rtOpts.s = DEFAULT_DELAY_S;
	rtOpts.inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
	rtOpts.outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
	rtOpts.max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;
	// rtOpts.ethernet_mode = FALSE;
	// rtOpts.offset_first_updated = FALSE;
	// rtOpts.file[0] = 0;
	rtOpts.maxDelayAutoTune = FALSE;
	rtOpts.discardedPacketThreshold = 60;
	rtOpts.logFd = -1;
	rtOpts.recordFP = NULL;
	rtOpts.do_log_to_file = FALSE;
	rtOpts.do_record_quality_file = FALSE;
	rtOpts.nonDaemon = FALSE;

	/*
	 * defaults for new options
	 */
	rtOpts.slaveOnly = TRUE;
	rtOpts.ignore_delayreq_interval_master = FALSE;
	rtOpts.do_IGMP_refresh = TRUE;
	rtOpts.useSysLog       = TRUE;
	rtOpts.syslog_startup_messages_also_to_stdout = TRUE;		/* used to print inital messages both to syslog and screen */
	rtOpts.announceReceiptTimeout  = DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
#ifdef RUNTIME_DEBUG
	rtOpts.debug_level = LOG_INFO;			/* by default debug messages as disabled, but INFO messages and below are printed */
#endif

	rtOpts.ttl = 1;
	rtOpts.delayMechanism   = DEFAULT_DELAY_MECHANISM;
	rtOpts.noResetClock     = DEFAULT_NO_RESET_CLOCK;
	rtOpts.log_seconds_between_message = 0;

	rtOpts.initial_delayreq = DEFAULT_DELAYREQ_INTERVAL;
	rtOpts.subsequent_delayreq = DEFAULT_DELAYREQ_INTERVAL;      // this will be updated if -g is given

#if 0
	/* Initialize run time options with command line arguments */
	if (!(ptpClock = ptpdStartup(argc, argv, &ret, &rtOpts)))
		return ret;

	/* global variable for message(), please see comment on top of this file */
	G_ptpClock = ptpClock;

	/* do the protocol engine */
	protocol(&rtOpts, ptpClock);
	/* forever loop.. */

	ptpdShutdown(ptpClock);
#endif

	NOTIFY("self shutdown, probably due to an error\n");

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


