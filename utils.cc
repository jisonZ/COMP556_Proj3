#include "utils.h"
#include <cstring>

void changeDVPacketToPacketInfo(void *start, DVL &pktInfo)
{
  /*
   * start: The DV packet,
   * size : How many bytes of the DV packet
   */

  unsigned short *packet_start = (unsigned short *)start; //
  unsigned short size = ntohs(*(packet_start + 1));       // get size of DV packet
  size -= 4;                                              // delete first row (packet type, reserved, size)
  size /= 4;                                              // return number of rows of DV data
  unsigned short first, second;
  size_t i;
  for (i = 0; i < size; ++i)
  {
    first = ntohs(*((unsigned short *)start + 2 + i * 2));
    second = ntohs(*((unsigned short *)start + 2 + i * 2 + 1));
    pktInfo.push_back(static_cast<PacketInfo>(make_pair(first, second)));
  }
  return;
};

ePacketType getPktType(void *packet)
{
  return (ePacketType)(*((unsigned char *)packet));
};

bool getLSinfo(void *packet, unordered_map<router_id, cost_time> &LSlist, seq_num &seqNum, router_id &routerID)
{
  /*
   * start: The LS packet,
   * size : How many bytes of the LS packet
   */
  //cout << "LS"<< endl;
  unsigned short *packet_start = (unsigned short *)packet; // get start of packet
  unsigned short size = ntohs(*(packet_start + 1));    
  //cout << "Packet Size: " << sizeof((char*)packet) << endl();    // get size of DV packet
  //size_t psize = strlen((char*)packet);
  //cout << "size:  " << psize << endl;
  if (size <= 15 || size-12 % 4 != 0) return false;

  routerID = ntohs(*(packet_start + 2));
  seqNum = ntohl(*((unsigned int *)(packet_start + 4)));
  //cout << "size:" << size;
  
  size -= 12; // delete first row (packet type, reserved, size)
  size /= 4;  // return number of rows of DV data
  unsigned short first, second;
  size_t i;
  for (i = 0; i < size; ++i)
  {
    first = ntohs(*((unsigned short *)packet + 6 + i * 2));
    second = ntohs(*((unsigned short *)packet + 6 + i * 2 + 1));
   //cout << first << " " << second << endl;
    LSlist[first] = second;
  }
  //cout << "LS2" << endl;
  return true;
};

void testPktType(ePacketType t)
{
  cout << "TEST PKT TYPE" << endl;
  switch (t)
  {
  case DATA:
    cout << "received DATA" << endl;
    break;
  case PING:
  case PONG:
    cout << "received PING/PONG" << endl;
    break;
  case DV:
    cout << "received DV" << endl;
    break;
  case LS:
    cout << "received LS" << endl;
    break;
  default:
    cout << "unexpected MSG type" << endl;
  }
};