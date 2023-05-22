/*-
 * class to transport ptp message over ethernet, udp/ipv4, udp/ipv6
 *
 */


#include "common.h"


//constructor
transport::transport(ptpd *pApp)
{
    BASE_MEMBER_ASSIGN 
}

unsigned char transport::reverse_8b(unsigned char data)
{
    unsigned char ra[8];
    int shift;

    for(int i = 0; i < 8; i++)
    {
      shift = 7 - i;
      ra[i] = (data >> shift) & 0x1;
    }

    unsigned char reverse = 0;
    for(int i = 0; i < 8; i++)
      reverse = reverse | (ra[i] << i);

    return reverse;
}

uint32_t transport::reverse_32b(uint32_t data)
{
    uint32_t ra[32];
    int shift;

    for(int i = 0; i < 32; i++)
    {
      shift = 31 - i;
      ra[i] = (data >> shift) & 0x1;
    }

    uint32_t reverse = 0;
    for(int i = 0; i < 32; i++)
      reverse = reverse | (ra[i] << i);

    return reverse;
}

uint32_t transport::nextCRC32_D8(unsigned char data, uint32_t currentCRC)
{
    uint32_t     data_in[8];
    uint32_t     lfsr_q[32];
    uint32_t     lfsr_c[32];

    for(int i = 0; i < 8; i++)
      data_in[i] = uint32_t(data >> i) & 0x1;

    for(int i = 0; i < 32; i++)
      lfsr_q[i] = (currentCRC >> i) & 0x1;

    lfsr_c[0] = lfsr_q[24] ^ lfsr_q[30] ^ data_in[0] ^ data_in[6];
    lfsr_c[1] = lfsr_q[24] ^ lfsr_q[25] ^ lfsr_q[30] ^ lfsr_q[31] ^ data_in[0] ^ data_in[1] ^ data_in[6] ^ data_in[7];
    lfsr_c[2] = lfsr_q[24] ^ lfsr_q[25] ^ lfsr_q[26] ^ lfsr_q[30] ^ lfsr_q[31] ^ data_in[0] ^ data_in[1] ^ data_in[2]
                ^ data_in[6] ^ data_in[7];
    lfsr_c[3] = lfsr_q[25] ^ lfsr_q[26] ^ lfsr_q[27] ^ lfsr_q[31] ^ data_in[1] ^ data_in[2] ^ data_in[3] ^ data_in[7];
    lfsr_c[4] = lfsr_q[24] ^ lfsr_q[26] ^ lfsr_q[27] ^ lfsr_q[28] ^ lfsr_q[30] ^ data_in[0] ^ data_in[2] ^ data_in[3] 
                ^ data_in[4] ^ data_in[6];
    lfsr_c[5] = lfsr_q[24] ^ lfsr_q[25] ^ lfsr_q[27] ^ lfsr_q[28] ^ lfsr_q[29] ^ lfsr_q[30] ^ lfsr_q[31] ^ data_in[0]
                ^ data_in[1] ^ data_in[3] ^ data_in[4] ^ data_in[5] ^ data_in[6] ^ data_in[7];
    lfsr_c[6] = lfsr_q[25] ^ lfsr_q[26] ^ lfsr_q[28] ^ lfsr_q[29] ^ lfsr_q[30] ^ lfsr_q[31] ^ data_in[1] ^ data_in[2]
                ^ data_in[4] ^ data_in[5] ^ data_in[6] ^ data_in[7];
    lfsr_c[7] = lfsr_q[24] ^ lfsr_q[26] ^ lfsr_q[27] ^ lfsr_q[29] ^ lfsr_q[31] ^ data_in[0] ^ data_in[2] ^ data_in[3] 
                ^ data_in[5] ^ data_in[7];
    lfsr_c[8] = lfsr_q[0] ^ lfsr_q[24] ^ lfsr_q[25] ^ lfsr_q[27] ^ lfsr_q[28] ^ data_in[0] ^ data_in[1] ^ data_in[3] ^ data_in[4];
    lfsr_c[9] = lfsr_q[1] ^ lfsr_q[25] ^ lfsr_q[26] ^ lfsr_q[28] ^ lfsr_q[29] ^ data_in[1] ^ data_in[2] ^ data_in[4] ^ data_in[5];
    lfsr_c[10] = lfsr_q[2] ^ lfsr_q[24] ^ lfsr_q[26] ^ lfsr_q[27] ^ lfsr_q[29] ^ data_in[0] ^ data_in[2] ^ data_in[3] ^ data_in[5];
    lfsr_c[11] = lfsr_q[3] ^ lfsr_q[24] ^ lfsr_q[25] ^ lfsr_q[27] ^ lfsr_q[28] ^ data_in[0] ^ data_in[1] ^ data_in[3] ^ data_in[4];
    lfsr_c[12] = lfsr_q[4] ^ lfsr_q[24] ^ lfsr_q[25] ^ lfsr_q[26] ^ lfsr_q[28] ^ lfsr_q[29] ^ lfsr_q[30] ^ data_in[0] ^ data_in[1]
                 ^ data_in[2] ^ data_in[4] ^ data_in[5] ^ data_in[6];
    lfsr_c[13] = lfsr_q[5] ^ lfsr_q[25] ^ lfsr_q[26] ^ lfsr_q[27] ^ lfsr_q[29] ^ lfsr_q[30] ^ lfsr_q[31] ^ data_in[1] 
                 ^ data_in[2] ^ data_in[3] ^ data_in[5] ^ data_in[6] ^ data_in[7];
    lfsr_c[14] = lfsr_q[6] ^ lfsr_q[26] ^ lfsr_q[27] ^ lfsr_q[28] ^ lfsr_q[30] ^ lfsr_q[31] ^ data_in[2] ^ data_in[3]
                 ^ data_in[4] ^ data_in[6] ^ data_in[7];
    lfsr_c[15] = lfsr_q[7] ^ lfsr_q[27] ^ lfsr_q[28] ^ lfsr_q[29] ^ lfsr_q[31] ^ data_in[3] ^ data_in[4]
                 ^ data_in[5] ^ data_in[7];
    lfsr_c[16] = lfsr_q[8] ^ lfsr_q[24] ^ lfsr_q[28] ^ lfsr_q[29] ^ data_in[0] ^ data_in[4] ^ data_in[5];
    lfsr_c[17] = lfsr_q[9] ^ lfsr_q[25] ^ lfsr_q[29] ^ lfsr_q[30] ^ data_in[1] ^ data_in[5] ^ data_in[6];
    lfsr_c[18] = lfsr_q[10] ^ lfsr_q[26] ^ lfsr_q[30] ^ lfsr_q[31] ^ data_in[2] ^ data_in[6] ^ data_in[7];
    lfsr_c[19] = lfsr_q[11] ^ lfsr_q[27] ^ lfsr_q[31] ^ data_in[3] ^ data_in[7];
    lfsr_c[20] = lfsr_q[12] ^ lfsr_q[28] ^ data_in[4];
    lfsr_c[21] = lfsr_q[13] ^ lfsr_q[29] ^ data_in[5];
    lfsr_c[22] = lfsr_q[14] ^ lfsr_q[24] ^ data_in[0];
    lfsr_c[23] = lfsr_q[15] ^ lfsr_q[24] ^ lfsr_q[25] ^ lfsr_q[30] ^ data_in[0] ^ data_in[1] ^ data_in[6];
    lfsr_c[24] = lfsr_q[16] ^ lfsr_q[25] ^ lfsr_q[26] ^ lfsr_q[31] ^ data_in[1] ^ data_in[2] ^ data_in[7];
    lfsr_c[25] = lfsr_q[17] ^ lfsr_q[26] ^ lfsr_q[27] ^ data_in[2] ^ data_in[3];
    lfsr_c[26] = lfsr_q[18] ^ lfsr_q[24] ^ lfsr_q[27] ^ lfsr_q[28] ^ lfsr_q[30] ^ data_in[0] ^ data_in[3] ^ data_in[4] ^ data_in[6];
    lfsr_c[27] = lfsr_q[19] ^ lfsr_q[25] ^ lfsr_q[28] ^ lfsr_q[29] ^ lfsr_q[31] ^ data_in[1] ^ data_in[4] ^ data_in[5] ^ data_in[7];
    lfsr_c[28] = lfsr_q[20] ^ lfsr_q[26] ^ lfsr_q[29] ^ lfsr_q[30] ^ data_in[2] ^ data_in[5] ^ data_in[6];
    lfsr_c[29] = lfsr_q[21] ^ lfsr_q[27] ^ lfsr_q[30] ^ lfsr_q[31] ^ data_in[3] ^ data_in[6] ^ data_in[7];
    lfsr_c[30] = lfsr_q[22] ^ lfsr_q[28] ^ lfsr_q[31] ^ data_in[4] ^ data_in[7];
    lfsr_c[31] = lfsr_q[23] ^ lfsr_q[29] ^ data_in[5];

    uint32_t nextCRC = 0;
    for(int i = 0; i < 32; i++)
      nextCRC = nextCRC | (lfsr_c[i] << i);

    return nextCRC;
}

uint32_t transport::calculate_crc(int data_len, unsigned char *frame_mem)
{
    uint32_t current_crc;
    uint32_t next_crc;

    current_crc = 0xffffffff;
    for(int i = 0; i < data_len; i++)
    {
      next_crc = nextCRC32_D8(reverse_8b(frame_mem[i]), current_crc);
      current_crc = next_crc; 
    }

    return  ~reverse_32b(current_crc);
}


int transport::assemble_frame(unsigned char *msg_buf, uint16_t msg_len, uint16_t length_type, uint16_t ether2_type, 
                     int vlan_tag, uint16_t udp_dport)
{

    int len;
    int m;
    int pad_len;
    int len_sub_vlan;
    uint32_t frame_crc;

    len = 0;
    //preamble
    for(int i = 0; i < 7; i++) {
      m_frame_mem[len++] = 0x55;
    }
    m_frame_mem[len++] = 0xd5;
    
    //destination mac address
    for(int i = 0; i < 6; i++) {
      m_frame_mem[len++] = m_mac_da[i];
    }

    //source mac address
    for(int i = 0; i < 6; i++) {
      m_frame_mem[len++] = m_mac_sa[i];
    }

    //external vlan tag
    if(vlan_tag == 2) {
      m_frame_mem[len++] = 0x88;  m_frame_mem[len++] = 0xa8;  
      m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x02;
    }
    //inner vlan tag or single vlan
    if(vlan_tag == 1 || vlan_tag == 2) {
      m_frame_mem[len++] = 0x81;  m_frame_mem[len++] = 0x00;  
      m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x02;
    }

    //ethernet length/type field
    m_frame_mem[len++] = (length_type >> 8) & 0xff;  
    m_frame_mem[len++] = length_type & 0xff;

    //802.3/snap header
    if(length_type <= 1500) {
      //snap header
      m_frame_mem[len++] = 0xaa;  m_frame_mem[len++] = 0xaa;  m_frame_mem[len++] = 0x00;  
      m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  
      m_frame_mem[len++] = (ether2_type >> 8) & 0xff;  
      m_frame_mem[len++] = ether2_type & 0xff;
    }
    else if(length_type == 0x8864) {
      //pppoe header
      m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  
      m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  
      m_frame_mem[len++] = (ether2_type >> 8) & 0xff;  
      m_frame_mem[len++] = ether2_type & 0xff;
    }

    //ipv4 header
    if(length_type == 0x0800 || (length_type < 1500 && ether2_type == 0x0800) || (length_type == 0x8864 && ether2_type == 0x0021)) {
      uint16_t total_len = 20 + 8 + msg_len;  //IP header length, UDP header length, message length
      m_frame_mem[len++] = 0x45;  m_frame_mem[len++] = 0x00;  
      m_frame_mem[len++] = (total_len >> 8) & 0xff;  m_frame_mem[len++] = total_len & 0xff;
      m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;
      m_frame_mem[len++] = 0xff;  m_frame_mem[len++] = 0x17;  m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;
      
      for(int i = 0; i < 4; i++) {
        m_frame_mem[len++] = m_ipv4_sa[i];
      }

      for(int i = 0; i < 4; i++) {
        m_frame_mem[len++] = m_ipv4_da[i];
      }
    }

    //ipv6 header
    if(length_type == 0x86dd || (length_type < 1500 && ether2_type == 0x86dd) ||(length_type == 0x8864 && ether2_type == 0x0057)) {
      m_frame_mem[len++] = 0x60;  m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;
      uint16_t payload_len = 8 + msg_len + 2; //UDP header length + PTP message length + padding
      m_frame_mem[len++] = (payload_len >> 8) & 0xff;  m_frame_mem[len++] = payload_len & 0xff;  
      m_frame_mem[len++] = 0x17;  m_frame_mem[len++] = 0xff;

      for(int i = 0; i < 16; i++) {
        m_frame_mem[len++] = m_ipv6_sa[i];
      }

      for(int i = 0; i < 16; i++) {
        m_frame_mem[len++] = m_ipv6_da[i];
      }
    }

    //udp header
    if(length_type == 0x0800 || length_type == 0x86dd || (length_type < 1500 && (ether2_type == 0x0800 || ether2_type == 0x86dd))
          || (length_type == 0x8864 && (ether2_type == 0x0021 || ether2_type == 0x0057))) {
      m_frame_mem[len++] = 0x00;  m_frame_mem[len++] = 0x00;  
      m_frame_mem[len++] = (udp_dport >> 8) & 0xff;  m_frame_mem[len++] = udp_dport & 0xff;

      uint16_t udp_len = 8 + msg_len;  //udp header length + ptp message length
      if(length_type == 0x86dd || (length_type < 1500 &&  ether2_type == 0x86dd) 
          || (length_type == 0x8864 && ether2_type == 0x0057)) {
        udp_len = udp_len + 2;    //+ padding
      }
      m_frame_mem[len++] = (udp_len >> 8) & 0xff; m_frame_mem[len++] = udp_len & 0xff;  

      m_frame_mem[len++] = 0xab; m_frame_mem[len++] = 0xcd;
    }

    //ptpv2 message payload
    for(int i = 0; i < msg_len; i++) {
      m_frame_mem[len++] = msg_buf[i];
    }

    //padding octets
    len_sub_vlan = (vlan_tag == 2) ? len-8 : ((vlan_tag == 1) ? len-4 : len);
    pad_len = 64 - (len_sub_vlan-8+4);
    if(length_type == 0x86dd || (length_type < 1500 &&  ether2_type == 0x86dd) || (length_type == 0x8864 && ether2_type == 0x0057)) {
      pad_len = 2;
    }

    if(pad_len > 0) {
      for(m = 0; m < pad_len; m = m+1) {
        m_frame_mem[len++] = 0x00;
      }
    }

    //crc octets
    frame_crc = calculate_crc(len-8, m_frame_mem+8);

    m_frame_mem[len++] = frame_crc & 0xff;
    m_frame_mem[len++] = (frame_crc >> 8) & 0xff;
    m_frame_mem[len++] = (frame_crc >> 16) & 0xff;
    m_frame_mem[len++] = (frame_crc >> 24) & 0xff;
    
    return len;
}
