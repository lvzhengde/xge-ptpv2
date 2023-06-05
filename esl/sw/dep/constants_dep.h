
/* constants_dep.h */

#ifndef CONSTANTS_DEP_H
#define CONSTANTS_DEP_H

/**
*\file
* \brief Plateform-dependent constants definition
*
* This header defines all includes and constants which are plateform-dependent
*
* ptpdv2 is only implemented for linux, NetBSD and FreeBSD
 */

/* platform dependent */


#include <climits>

#define IFACE_NAME_LENGTH         128
#define MAXHOSTNAMELEN            128

#define DEFAULT_IFACE_NAME        "eth-ptpv2"

#define CLOCK_IDENTITY_LENGTH 8

/* UDP/IPv4 dependent */
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001UL
#endif

#define SUBDOMAIN_ADDRESS_LENGTH  4
#define PORT_ADDRESS_LENGTH       2
#define PTP_UUID_LENGTH           6
#define CLOCK_IDENTITY_LENGTH	  8
#define FLAG_FIELD_LENGTH         2

#define PACKET_SIZE  300 //ptpdv1 value kept because of use of TLV...

#define PTP_EVENT_PORT    319
#define PTP_GENERAL_PORT  320

#define MM_STARTING_BOUNDARY_HOPS  0x7fff

/* others */

/* bigger screen size constants */
#define SCREEN_BUFSZ  228
#define SCREEN_MAXSZ  180

/* default size for string buffers */
#define BUF_SIZE  1000


#define NANOSECONDS_MAX 999999999

#define SECONDS_MAX     ((1<<48)-1)

#define CLOCK_PERIOD   (6.4)     //unit in nanoseconds

#define FREQ_VARIANCE  (100)     //in PPM

#define DOT_POS        (26)      //position of fractional point

#define INITIAL_TICK   ((uint32_t)(CLOCK_PERIOD * (1 << DOT_POS) + 0.5))

#define ADJ_FREQ_MAX   ((int32_t)(FREQ_VARIANCE * (1e-6) * CLOCK_PERIOD * (1 << DOT_POS) + 0.5))

#define PI_UNIT        (1e9/(CLOCK_PERIOD * (1 << DOT_POS)) / 2.0)   //corresponding to f_clk/2

#define DEFAULT_AP     (3.0 * PI_UNIT)  

#define DEFAULT_AI     (9.0 * PI_UNIT)       

#define DEFAULT_DELAY_S    	6

// limit operator messages to once every X seconds
#define OPERATOR_MESSAGES_INTERVAL 300.0


#define MAXTIMESTR 32

//#define LOG_DAEMON 0
#define NET_ADDRESS_LENGTH 16 /* for IPv4 dotted-decimal */

#endif /*CONSTANTS_DEP_H_*/
