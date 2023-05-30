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
 * @file   startup.c
 * @date   Wed Jun 23 09:33:27 2010
 * 
 * @brief  Code to handle daemon startup, including command line args
 * 
 * The function in this file are called when the daemon starts up
 * and include the getopt() command line argument parsing.
 */

#include "common.h"

/*
 * valgrind 3.5.0 currently reports no errors (last check: 20110512)
 * valgrind 3.4.1 lacks an adjtimex handler
 *
 * to run:   sudo valgrind --show-reachable=yes --leak-check=full --track-origins=yes -- ./ptpd2 -c ...
 */

/*
  to test daemon locking and startup sequence itself, try:

  function s()  { set -o pipefail ;  eval "$@" |  sed 's/^/\t/' ; echo $?;  }
  sudo killall ptpd2
  s ./ptpd2
  s sudo ./ptpd2
  s sudo ./ptpd2 -t -g
  s sudo ./ptpd2 -t -g -b eth0
  s sudo ./ptpd2 -t -g -b eth0
  ps -ef | grep ptpd2
*/

//constructor
startup::startup(ptpd *pApp)
{
    BASE_MEMBER_ASSIGN 

    sigint_received  = 0;
    sigterm_received = 0;
    sighup_received  = 0;
    sigusr1_received = 0;
    sigusr2_received = 0;
}

/**
 * Stub function (should defined as static function in real application)
 * Function to catch signals asynchronously.
 * Assuming that the daemon periodically calls check_signals(), then all operations are safely done synchrously at a later opportunity.
 *
 * Please do NOT call any functions inside this handler - especially DBG() and its friends, or any glibc.
 */
void startup::catch_signals(int sig)
{
	switch (sig) {
	case SIGINT:
		sigint_received = 1;
		break;
	case SIGTERM:
		sigterm_received = 1;
		break;
	case SIGHUP:
		sighup_received = 1;
		break;
	case SIGUSR1:
		sigusr1_received = 1;
		break;
	case SIGUSR2:
		sigusr2_received = 1;
		break;
	default:
		/*
		 * TODO: should all other signals be catched, and handled as SIGINT?
		 *
		 * Reason: currently, all other signals are just uncatched, and the OS kills us.
		 * The difference is that we could then close the open files properly.
		 */
		break;
	}
}


/*
 * exit the program cleanly
 */
void
startup::do_signal_close(PtpClock * ptpClock)
{
	ptpdShutdown(ptpClock);

	NOTIFY("shutdown on close signal\n");
	exit(0);
}

/** 
 * Signal handler for HUP which tells us to swap the log file.
 * 
 * @param sig 
 */
void 
startup::do_signal_sighup(RunTimeOpts * rtOpts)
{
	if(rtOpts->do_record_quality_file)
	if(!recordToFile(rtOpts))
		NOTIFY("SIGHUP recordToFile failed\n");

	if(rtOpts->do_log_to_file)
		if(!logToFile(rtOpts))
			NOTIFY("SIGHUP logToFile failed\n");

	NOTIFY("I've been SIGHUP'd\n");
}


/*
 * Synchronous signal processing:
 * This function should be called regularly from the main loop
 */
void
startup::check_signals(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	/*
	 * note:
	 * alarm signals are handled in a similar way in dep/timer.c
	 */

	if(sigint_received || sigterm_received){
		do_signal_close(ptpClock);
	}

	if(sighup_received){
		do_signal_sighup(rtOpts);
	sighup_received=0;
	}

	if(sigusr1_received){
		WARNING("SigUSR1 received, manually stepping clock to current known OFM\n");
		m_pApp->m_ptr_servo->servo_perform_clock_step(rtOpts, ptpClock);
	sigusr1_received = 0;
	}


#ifdef DBG_SIGUSR2_CHANGE_DOMAIN
	if(sigusr2_received){
		/* swap domain numbers */
		static int prev_domain;
		static int first_time = 1;
		if(first_time){
			first_time = 0;
			prev_domain = ptpClock->domainNumber + 1;
		}

		int  temp = ptpClock->domainNumber;
		ptpClock->domainNumber = prev_domain;
		prev_domain = temp;

		
		// propagate new choice as the run-time option
		rtOpts->domainNumber = ptpClock->domainNumber;
		
		WARNING("SigUSR2 received. PTP_Domain is now %d  (saved: %d)\n",
			ptpClock->domainNumber,
			prev_domain
		);
		sigusr2_received = 0;
	}
#endif

		
#ifdef DBG_SIGUSR2_CHANGE_DEBUG
#ifdef RUNTIME_DEBUG
	if(sigusr2_received){
		/* cycle debug levels, from INFO (=no debug) to Verbose */
		INFO("Current debug level: %d\n", rtOpts->debug_level);
		
		(rtOpts->debug_level)++;
		if(rtOpts->debug_level > LOG_DEBUGV ){
			rtOpts->debug_level = LOG_INFO;
	}

		INFO("New debug level: %d\n", rtOpts->debug_level);
	sigusr2_received = 0;
	}
#endif
#endif
	
}

#ifdef RUNTIME_DEBUG
/* These functions are useful to temporarily enable Debug around parts of code, similar to bash's "set -x" */
void startup::enable_runtime_debug(void )
{
	extern RunTimeOpts rtOpts;
	
	rtOpts.debug_level = max(LOG_DEBUGV, rtOpts.debug_level);
}

void startup::disable_runtime_debug(void )
{
	extern RunTimeOpts rtOpts;
	
	rtOpts.debug_level = LOG_INFO;
}
#endif

/*
 * Lock via filesystem implementation, as described in "Advanced Programming in the UNIX Environment, 2nd ed"
 */

/* try to apply a write lock on the file */
int
startup::lockfile(int fd)
{
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return(fcntl(fd, F_SETLK, &fl));
}


/*
 * a discussion where the lock file should reside: http://rute.2038bug.com/node38.html.gz#SECTION003859000000000000000
 */
int
startup::daemon_already_running(void)
{
	return 0;
}


/* return number of pgrep exact matches to the given string
 * -1: error
 * 0: no matches
 * >0: number of matches
 */
int
startup::pgrep_matches(char *name)
{
	char command[BUF_SIZE];
	char answer[BUF_SIZE];
	int matches;

	/* use pgrep to count processes with the given name
	 * but exclude our own pid from the search results */
	snprintf(command, BUF_SIZE - 1, "pgrep -x %s | grep -v ^%ld$ | wc -l", name, (long)getpid());

	if( query_shell(command, answer, BUF_SIZE) < 0){
		return -1;
	};
	
	sscanf(answer, "%d", &matches);
	return (matches);
}

/*
 * This function executes a given shell command (including pipes), and returns the first line of the output
 * these methods are only suitable for initialization time :)
 *
 *
 * ret: -1 error ; 0 ok
 *
 */
int startup::query_shell(char *command, char *answer, int answer_size)
{
	int status;
	FILE *fp;

	// clear previous answer
	answer[0] = '\0';

	fp = popen(command, "r");
	if (fp == NULL){
		PERROR("can't call  %s", command);
		return -1;
	};

	// get first line of popen
	fgets(answer, answer_size - 1, fp);

	DBG2("Query_shell: _%s_ -> _%s_\n", command, answer);

	status = pclose(fp);
	if (status == -1) {
		PERROR("can't call pclose() ");
		return -1;
	} 

	/* from Man page:
	     Use macros described under wait() to inspect `status' in order
	     to determine success/failure of command executed by popen() */

	return 0;
}

/*
 * expected: number of expected deamons
 * strict: if it doesn't match what is expected, should we exit?
 *
 * return:
 *   1: ok
 *   0: error, will exit program
 */
int
startup::check_parallel_daemons(string name, int expected, int strict, RunTimeOpts * rtOpts)
{
	return 1;
}


/** 
 * Log output to a file
 * 
 * 
 * @return True if success, False if failure
 */
int 
startup::logToFile(RunTimeOpts * rtOpts)
{
	if(rtOpts->logFd != -1)
		close(rtOpts->logFd);
	

	/* We new append the file instead of replacing it. Also use mask 644 instead of 444 */
	if((rtOpts->logFd = open(rtOpts->file, O_CREAT | O_APPEND | O_RDWR, 0644 )) != -1) {
		dup2(rtOpts->logFd, STDOUT_FILENO);
		dup2(rtOpts->logFd, STDERR_FILENO);
	}
	return rtOpts->logFd != -1;
}

/** 
 * Record quality data for later correlation
 * 
 * 
 * @return True if success, False if failure
 */
int
startup::recordToFile(RunTimeOpts * rtOpts)
{
	if (rtOpts->recordFP != NULL)
		fclose(rtOpts->recordFP);

	if ((rtOpts->recordFP = fopen(rtOpts->recordFile, "w")) == NULL)
		PERROR("could not open sync recording file");
	else
		setlinebuf(rtOpts->recordFP);
	return (rtOpts->recordFP != NULL);
}


void 
startup::ptpdShutdown(PtpClock * ptpClock)
{
	m_pApp->m_ptr_net->netShutdown(&ptpClock->netPath);
	free(ptpClock->foreign);

	/* free management messages, they can have dynamic memory allocated */
	if(ptpClock->msgTmpHeader.messageType == MANAGEMENT)
		m_pApp->m_ptr_msg->freeManagementTLV(&ptpClock->msgTmp.manage);
	m_pApp->m_ptr_msg->freeManagementTLV(&ptpClock->outgoingManageTmp);

	free(ptpClock);
	ptpClock = NULL;

	m_pApp->m_ptr_ptpClock = NULL;
}


void startup::dump_command_line_parameters(int argc, char **argv)
{
	//
	int i = 0;
	char sbuf[1000];
	char *st = sbuf;
	int len = 0;
	
	*st = '\0';
	for(i=0; i < argc; i++){
		len += snprintf(sbuf + len,
					     sizeof(sbuf) - len,
					     "%s ", argv[i]);
	}

	INFO("\n");
	INFO("Starting %s daemon with parameters:      %s\n", PTPD_PROGNAME, sbuf);
}


void
startup::display_short_help(string error)
{
	printf(
			//"\n"
			PTPD_PROGNAME ":\n"
			"   -gGW          Protocol mode (slave/master with ntp/master without ntp)\n"
			"   -b <dev>      Interface to use\n"
			"\n"
			"   -cC  -DVfS    Console / verbose console;     Dump stats / Interval / Output file / no Syslog\n"
			"   -uU           Unicast/Hybrid mode\n"
			"\n"
			"\n"
			"   -hHB           Summary / Complete help file / run-time debug level\n"
			"\n"
			"ERROR: %s\n\n",
			error.c_str()
		);
}

PtpClock *
startup::ptpdStartup(int argc, char **argv, Integer16 * ret, RunTimeOpts * rtOpts)
{
	PtpClock * ptpClock;
	int c, noclose = 0;
	int mode_selected = 0;		// 1: slave / 2: master, with ntp / 3: master without ntp

	int ntp_daemons_expected = 0;
	int ntp_daemons_strict = 1;
	int ptp_daemons_expected = 0;
	int ptp_daemons_strict = 1;

	dump_command_line_parameters(argc, argv);

    const char *getopt_string = "HgGWb:cCf:ST:DPR:xO:tM:a:w:u:Uehzl:o:i:I:n:N:y:m:v:r:s:p:q:Y:BjLV:A:";

	/* parse command line arguments */
	while ((c = getopt(argc, argv, getopt_string)) != -1) {
		switch (c) {
		case '?':
			printf("\n");
			display_short_help("Please input correct parameters (use -H for complete help file)");
			*ret = 1;
			return 0;
			break;
			
		case 'H':
			printf(
				"\nUsage:  ptpv2d [OPTION]\n\n"
				"Ptpv2d runs on Ethernet , P2P mode by default\n"
				"\n"
#define GETOPT_START_OF_OPTIONS
				"-H                show detailed help page\n"
				"\n"
				"Mode selection (one option is always required):\n"
				"-g                run as slave only\n"
				"-G                run as a Master _with_ NTP  (implies -t -L)\n"
				"-W                run as a Master _without_ NTP (reverts to client when inactive)\n"
				"-b NAME           bind PTP to network interface NAME\n"
				"\n"
				"Options:\n"
				"-c                run in command line (non-daemon) mode (implies -D)\n"
				"-C                verbose non-daemon mode: implies -c -S -D -V 0, disables -f\n"
				"-f FILE           send output to FILE (implies -D)\n"
				"-S                DON'T send messages to syslog\n"
				"\n"
				"-T NUMBER         set multicast time to live\n"
				"-D                display stats in .csv format (per received packet, see also -V)\n"
				"-P                display each received packet in detail\n"
				"-R FILE           record data about sync packets in a seperate file\n"
				"\n"
				"-x                do not reset the clock if off by more than one second\n"
				"-O NUMBER         do not reset the clock if offset is more than NUMBER nanoseconds\n"

				"-t                do not make any changes to the system clock\n"
				"-M NUMBER         do not accept delay values of more than NUMBER nanoseconds\n"
				"-a 10,1000        specify clock servo Proportional and Integral attenuations\n"
				"-w NUMBER         specify one way delay filter stiffness\n"
				"\n"
				"-u ADDRESS        Unicast mode: send all messages in unicast to ADDRESS\n"
				"-U                Hybrid  mode: send DELAY messages in unicast\n"
				"                    This causes all delayReq messages to be sent in unicast to the\n"
				"                    IP address of the Master (taken from the last announce received).\n"
				"                    For masters, it replyes the delayResp to the IP address of the client\n"
				"                    (from the corresponding delayReq message).\n"
				"                    All other messages are send in multicast\n"
				"\n"
				"-e                run in ethernet mode (level2) \n"
				"-h                run in End to End mode \n"
				"-z                run in Peer-delay mode\n"
				"-l NUMBER,NUMBER  specify inbound, outbound latency in nsec.\n"
				"                    (Use this to compensate the time it takes to send and recv\n"
				"                    messages via sockets)\n"
				"\n"
				"-o NUMBER         specify current UTC offset\n"
				"-i NUMBER         specify PTP domain number (between 0-3)\n"
				"-I NUMBER         specify Mcast group (between 0-3, emulates PTPv1 group selection)\n"
				
				"\n"
				"-n NUMBER         specify announce interval in 2^NUMBER sec\n"
				"-N NUMBER         specify announce receipt TO (number of lost announces to timeout)\n"

				"-y NUMBER         specify sync interval in 2^NUMBER sec\n"
				"-m NUMBER         specify max number of foreign master records\n"
				"\n"
				"-v NUMBER         Master mode: specify system clock Allen variance\n"
				"-r NUMBER         Master mode: specify system clock accuracy\n"
				"-s NUMBER         Master mode: specify system clock class\n"
				"-p NUMBER         Master mode: specify priority1 attribute\n"
				"-q NUMBER         Master mode: specify priority2 attribute\n"
				"\n"
				"\n"
				"-Y 0[,0]          Initial and Master_Overide delayreq intervals\n"
				"                     desc: the first 2^ number is the rate the slave sends delayReq\n"
				"                     When the first answer is received, the master value is used (unless the\n"
				"                     second number was also given)\n"
				"\n"
				"-B                Enable debug messages (if compiled-in). Multiple invocations to more debug\n"
				"\n"
				"Compatibility Options (to restore previous default behaviour):\n"
				"-j                Do not refresh the IGMP Multicast menbership at each protol reset\n"
				"-L                Allow multiple instances (ignore lock and other daemons)\n"
				"-V 0              Seconds between log messages (0: all messages)\n"
				"\n"
				"\n"


#define GETOPT_END_OF_OPTIONS
				"\n"
				"Possible internal states:\n"
				"  init:        INITIALIZING\n"
				"  flt:         FAULTY\n"
				"  lstn_init:   LISTENING (first time)\n"
				"  lstn_reset:  LISTENING (non first time)\n"
				"  pass:        INACTIVE Master\n"
				"  uncl:        UNCALIBRATED\n"
				"  slv:         SLAVE\n"
				"  pmst:        PRE Master\n"
				"  mst:         ACTIVE Master\n"
				"  dsbl:        DISABLED\n"
				"  ?:           UNKNOWN state\n"

				"\n"

				"Signals synchronous behaviour:\n"
				"  SIGHUP         Re-open statistics log (specified with -f)\n"
				"  SIGUSR1        Manually step clock to current OFM value (overides -x, but honors -t)\n"
				"  SIGUSR2        swap domain between current and current + 1 (useful for testing)\n"
				"  SIGUSR2        cycle run-time debug level (requires RUNTIME_DEBUG)\n"
				"\n"
				"  SIGINT|TERM    close file, remove lock file, and clean exit\n"
				"  SIGKILL|STOP   force an unclean exit\n"
				
				"\n"
				"BMC Algorithm defaults:\n"
				"  Software:   P1(128) > Class(13|248) > Accuracy(\"unk\"/0xFE)   > Variance(61536) > P2(128)\n"

											 
				/*  "-k NUMBER,NUMBER  send a management message of key, record, then exit\n" implemented later.. */
			    "\n"
			    );
			*ret = 1;
			return 0;
			break;

		case 'c':
			rtOpts->nonDaemon = 1;
			break;
			
		case 'C':
			rtOpts->nonDaemon = 2;

			rtOpts->useSysLog    = FALSE;
			rtOpts->syslog_startup_messages_also_to_stdout = TRUE;
			rtOpts->displayStats = TRUE;
			rtOpts->log_seconds_between_message = 0;
			rtOpts->do_log_to_file = FALSE;
			break;
			
		case 'S':
			rtOpts->useSysLog = FALSE;
			break;
		case 'T':
			rtOpts->ttl = atoi(optarg);
			break;
		case 'f':
			/* this option handling is now after the arguments processing (to show the help in case of -?) */
			strncpy(rtOpts->file, optarg, PATH_MAX);
			rtOpts->do_log_to_file = TRUE;
			break;

		case 'B':
#ifdef RUNTIME_DEBUG
			(rtOpts->debug_level)++;
			if(rtOpts->debug_level > LOG_DEBUGV ){
				rtOpts->debug_level = LOG_DEBUGV;
			}
#else
			INFO("runtime debug not enabled. Please compile with RUNTIME_DEBUG\n");
#endif
			break;
			
		case 'D':
			rtOpts->displayStats = TRUE;
			break;

		case 'P':
			rtOpts->displayPackets = TRUE;
			break;

		case 'R':
			/* this option handling is now after the arguments processing (to show the help in case of -?) */
			strncpy(rtOpts->recordFile, optarg, PATH_MAX);
			rtOpts->do_record_quality_file = TRUE;
			break;


		case 'x':
			rtOpts->noResetClock = TRUE;
			break;
		case 'O':
			rtOpts->maxReset = atoi(optarg);
			if (rtOpts->maxReset > 1000000000) {
				ERROR_("Use -x to prevent jumps of more"
				       " than one second.");
				*ret = 1;
				return (0);
			}
			break;
		case 'A':
			rtOpts->maxDelayAutoTune = TRUE;
			rtOpts->discardedPacketThreshold = atoi(optarg);
			break;
		case 'M':
			rtOpts->maxDelay = rtOpts->origMaxDelay = atoi(optarg);
			if (rtOpts->maxDelay > 1000000000) {
				ERROR_("Use -x to prevent jumps of more"
				       " than one second.");
				*ret = 1;
				return (0);
			}
			break;
		case 't':
			rtOpts->noAdjust = TRUE;
			break;
		case 'a':
			rtOpts->ap = strtol(optarg, &optarg, 0);
			if (optarg[0])
				rtOpts->ai = strtol(optarg + 1, 0, 0);
			break;
		case 'w':
			rtOpts->s = strtol(optarg, &optarg, 0);
			break;
		case 'b':
			memset(rtOpts->ifaceName, 0, IFACE_NAME_LENGTH);
			strncpy(rtOpts->ifaceName, optarg, IFACE_NAME_LENGTH);
			break;

		case 'u':
			rtOpts->do_unicast_mode = 1;
			strncpy(rtOpts->unicastAddress, optarg, 
				MAXHOSTNAMELEN);

			/*
			 * FIXME: some code still relies on checking if this variable is filled. Upgrade this to do_unicast_mode
			 *
			 * E.g.:  netSendEvent(Octet * buf, UInteger16 length, NetPath * netPath, Integer32 alt_dst)
			 *  if(netPath->unicastAddr || alt_dst ){
			 */
			break;
			 
		case 'U':
#ifdef PTP_EXPERIMENTAL
			rtOpts->do_hybrid_mode = 1;
#else
			INFO("Hybrid mode not enabled. Please compile with PTP_EXPERIMENTAL\n");
#endif
			break;

			
		case 'l':
			rtOpts->inboundLatency.nanoseconds = 
				strtol(optarg, &optarg, 0);
			if (optarg[0])
				rtOpts->outboundLatency.nanoseconds = 
					strtol(optarg + 1, 0, 0);
			break;
		case 'o':
			rtOpts->currentUtcOffset = strtol(optarg, &optarg, 0);
			break;
		case 'i':
			rtOpts->domainNumber = strtol(optarg, &optarg, 0);
			break;

		case 'I':
#ifdef PTP_EXPERIMENTAL
			rtOpts->mcast_group_Number = strtol(optarg, &optarg, 0);
#else
			INFO("Multicast group selection not enabled. Please compile with PTP_EXPERIMENTAL\n");
#endif
			break;


			
		case 'y':
			rtOpts->syncInterval = strtol(optarg, 0, 0);
			break;
		case 'n':
			rtOpts->announceInterval = strtol(optarg, 0, 0);
			break;

		case 'N':
			rtOpts->announceReceiptTimeout = strtol(optarg, 0, 0);
			break;
			
		case 'm':
			rtOpts->max_foreign_records = strtol(optarg, 0, 0);
			if (rtOpts->max_foreign_records < 1)
				rtOpts->max_foreign_records = 1;
			break;
		case 'v':
			rtOpts->clockQuality.offsetScaledLogVariance = 
				strtol(optarg, 0, 0);
			break;
		case 'r':
			rtOpts->clockQuality.clockAccuracy = 
				strtol(optarg, 0, 0);
			break;
		case 's':
			rtOpts->clockQuality.clockClass = strtol(optarg, 0, 0);
			break;
		case 'p':
			rtOpts->priority1 = strtol(optarg, 0, 0);
			break;
		case 'q':
			rtOpts->priority2 = strtol(optarg, 0, 0);
			break;
		case 'e':
			rtOpts->ethernet_mode = TRUE;
			ERROR_("Not implemented yet !");
			return 0;
			break;
		case 'h':
			rtOpts->delayMechanism = E2E;
			break;

		case 'V':
			rtOpts->log_seconds_between_message = strtol(optarg, &optarg, 0);
			break;

		case 'z':
			rtOpts->delayMechanism = P2P;
			break;

		/* mode selection */
		/* slave only */
		case 'g':
			mode_selected = 1;
			rtOpts->slaveOnly = TRUE;
			rtOpts->initial_delayreq = DEFAULT_DELAYREQ_INTERVAL;		// allow us to use faster rate at init

			/* we dont expect any parallel deamons, and we are strict about it  */
			ntp_daemons_expected = 0;
			ntp_daemons_strict = 1;
			ptp_daemons_expected = 0;
			ptp_daemons_strict = 1;
			break;
			
		/* Master + NTP */
		case 'G':
			mode_selected = 2;
			rtOpts->clockQuality.clockClass = DEFAULT_CLOCK_CLASS__APPLICATION_SPECIFIC_TIME_SOURCE;
			rtOpts->slaveOnly = FALSE;
			rtOpts->noAdjust = TRUE;

			/* we expect one ntpd daemon in parallel, but we are not strick about it */
			ntp_daemons_expected = 1;
			ntp_daemons_strict = 0;
			ptp_daemons_expected = 0;
			ptp_daemons_strict = 0;
			break;

		/*
		 *  master without NTP (Original Master behaviour):
		 *    it falls back to slave mode when its inactive master;
		 *    once it starts being active, it will drift for itself, so in actual terms it always requires NTP to work properly
		 */
		case 'W':
			mode_selected = 3;
			rtOpts->slaveOnly = FALSE;
			rtOpts->noAdjust = FALSE;

			/* we don't expect ntpd, but we can run with other ptpv1 deamons  */
			ntp_daemons_expected = 0;
			ntp_daemons_strict = 1;
			ptp_daemons_expected = 0;
			ptp_daemons_strict = 0;
			break;

			
		case 'Y':
			rtOpts->initial_delayreq = strtol(optarg, &optarg, 0);
			rtOpts->subsequent_delayreq = rtOpts->initial_delayreq;
			rtOpts->ignore_delayreq_interval_master = FALSE;

			/* Use this to override the master-given DelayReq value */
			if (optarg[0]){
				rtOpts->subsequent_delayreq = strtol(optarg + 1, &optarg, 0);
				rtOpts->ignore_delayreq_interval_master = TRUE;
			}
			break;

		case 'L':
			/* enable running multiple ptpd2 daemons */
			rtOpts->ignore_daemon_lock = TRUE;
			break;
		case 'j':
			rtOpts->do_IGMP_refresh = FALSE;
			break;
		
			

		default:
			ERROR_("Unknown parameter %c \n", c);
			*ret = 1;
			return 0;
		}
	}

    //clock mode determined by clock_id
	mode_selected = m_pController->m_clock_id;

	if(!mode_selected){
		display_short_help("Please select program mode");
		*ret = 1;
		return 0;
	}
	else if(mode_selected == 1){
	    rtOpts->slaveOnly = TRUE;
	}

	ptpClock = (PtpClock *) calloc(1, sizeof(PtpClock));
	if (!ptpClock) {
		PERROR("failed to allocate memory for protocol engine data");
		*ret = 2;
		return 0;
	} else {
		DBG("allocated %d bytes for protocol engine data\n", 
		    (int)sizeof(PtpClock));
		ptpClock->foreign = (ForeignMasterRecord *)
			calloc(rtOpts->max_foreign_records, 
			       sizeof(ForeignMasterRecord));
		if (!ptpClock->foreign) {
			PERROR("failed to allocate memory for foreign "
			       "master data");
			*ret = 2;
			free(ptpClock);
			return 0;
		} else {
			DBG("allocated %d bytes for foreign master data\n", 
			    (int)(rtOpts->max_foreign_records * 
				  sizeof(ForeignMasterRecord)));
		}
	}

	/* Init to 0 net buffer */
	memset(ptpClock->msgIbuf, 0, PACKET_SIZE);
	memset(ptpClock->msgObuf, 0, PACKET_SIZE);


	/* Init user_description */
	memset(ptpClock->user_description, 0, sizeof(ptpClock->user_description));
	memcpy(ptpClock->user_description, &USER_DESCRIPTION, sizeof(USER_DESCRIPTION));
	
	/* Init outgoing management message */
	ptpClock->outgoingManageTmp.tlv = NULL;

	/* Manage open files: stats and quality file */
	if(rtOpts->do_record_quality_file){
		if (recordToFile(rtOpts))
			noclose = 1;
		else
			PERROR("could not open quality file");
	}

	if(rtOpts->do_log_to_file){
		if(logToFile(rtOpts))
			noclose = 1;
		else
			PERROR("could not open output file");

		rtOpts->displayStats = TRUE;
	}

	/* if syslog is on, send all messages to syslog only  */
	rtOpts->syslog_startup_messages_also_to_stdout = FALSE;   

#if 0	
    //stub code for reference only
	/* use new synchronous signal handlers */
	signal(SIGINT,  startup::catch_signals);
	signal(SIGTERM, startup::catch_signals);
	signal(SIGHUP,  startup::catch_signals);
	signal(SIGUSR1, startup::catch_signals);
	signal(SIGUSR2, startup::catch_signals);
#endif

	*ret = 0;

	INFO("  Info:    Startup finished sucessfully\n");

	return ptpClock;
}
