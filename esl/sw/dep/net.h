#ifndef _NET_H__
#define _NET_H__

#include "datatypes.h"

static Boolean netShutdownMulticastIPv4(NetPath * netPath, Integer32 multicastAddr);

static Boolean netShutdownMulticast(NetPath * netPath);

Boolean netShutdown(NetPath * netPath);

Boolean chooseMcastGroup(RunTimeOpts * rtOpts, struct in_addr *netAddr);

UInteger8 lookupCommunicationTechnology(UInteger8 communicationTechnology);

UInteger32 findIface(Octet * ifaceName, UInteger8 * communicationTechnology,
    Octet * uuid, NetPath * netPath);

static Boolean netInitMulticastIPv4(NetPath * netPath, Integer32 multicastAddr);

Boolean netInitMulticast(NetPath * netPath,  RunTimeOpts * rtOpts);

Boolean netInitTimestamping(NetPath * netPath);

Boolean netInit(NetPath * netPath, RunTimeOpts * rtOpts, PtpClock * ptpClock);

int netSelect(TimeInternal * timeout, NetPath * netPath);

ssize_t netRecvEvent(Octet * buf, TimeInternal * time, NetPath * netPath);

ssize_t netRecvGeneral(Octet * buf, TimeInternal * time, NetPath * netPath);

ssize_t netSendEvent(Octet * buf, UInteger16 length, NetPath * netPath, Integer32 alt_dst);

ssize_t netSendGeneral(Octet * buf, UInteger16 length, NetPath * netPath, Integer32 alt_dst);

ssize_t netSendPeerGeneral(Octet * buf, UInteger16 length, NetPath * netPath);

ssize_t netSendPeerEvent(Octet * buf, UInteger16 length, NetPath * netPath);

Boolean netRefreshIGMP(NetPath * netPath, RunTimeOpts * rtOpts, PtpClock * ptpClock);

#endif // _NET_H__

