#ifndef _TRANSPORT_H__
#define _TRANSPORT_H__

#include "datatypes.h"

class transport : public base_data
{
public:
  //member variables
  unsigned char m_mac_sa[6];

  unsigned char m_mac_da[6];

  unsigned char m_ipv4_sa[4];

  unsigned char m_ipv4_da[4];

  unsigned char m_ipv6_sa[16];

  unsigned char m_ipv6_da[16];

public:
  //member methods
  transport(ptpd *pApp);

  unsigned char reverse_8b(unsigned char data);

  uint32_t reverse_32b(uint32_t data);

  uint32_t nextCRC32_D8(unsigned char data, uint32_t currentCRC);

  uint32_t calculate_crc(int data_len, unsigned char *frame_mem);

};

#endif // _TRANSPORT_H__

