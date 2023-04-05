#include "utils.h"

void changeDVPacketToPacketInfo(void *start, DVL &pktInfo)
{
  /*
   * start: The DV packet,
   * size : How many bytes of the DV packet
   */
  unsigned short *packet_start = (unsigned short *)start; //
  unsigned short size = ntohs(*(packet_start + 1));       // Get Size of DV packet
  size -= 4;                                              // Delete First Row (Packet Type, Reserved, Size)
  size /= 4;                                              // Return How Many Rows of DV data
  unsigned short first, second;
  size_t i;
  for (i = 0; i < size; ++i)
  {
    first = ntohs(*((unsigned short *)start + 2 + i * 2));
    second = ntohs(*((unsigned short *)start + 2 + i * 2 + 1));
    
    // cout << "change DV : " << first << " " << second << endl;
    pktInfo.push_back(static_cast<PacketInfo>(make_pair(first, second)));
  }
  return;
};

ePacketType getPktType(void *packet)
{
  return (ePacketType)(*((unsigned char *)packet));
};

void getLSinfo(void *packet, unordered_map<router_id, cost_time> &LSlist, seq_num &seqNum, router_id& routerID)
{
    /*
     * start: The LS packet,
     * size : How many bytes of the LS packet
     */
    unsigned short *packet_start = (unsigned short *)packet; //
    unsigned short size = ntohs(*(packet_start + 1));       // Get Size of DV packet
    routerID = ntohs(*(packet_start + 2));
    seqNum = ntohl(*((unsigned int*)(packet_start + 4)));
    
    size -= 12;                                              // Delete First Row (Packet Type, Reserved, Size)
    size /= 4;                                              // Return How Many Rows of DV data
    unsigned short first, second;
    size_t i;
    for (i = 0; i < size; ++i)
    {
        first = ntohs(*((unsigned short *)packet + 6 + i * 2));
        second = ntohs(*((unsigned short *)packet + 6 + i * 2 + 1));
        LSlist[first] = second;
    }
    return;
};