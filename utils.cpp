#include "utils.h"

void changeDVPacketToPacketInfo(void *start, vector<PacketInfo> &pktInfo)
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
    first = ntohs(*((unsigned short *)start + i * 2));
    second = ntohs(*((unsigned short *)start + i * 2 + 1));
    pktInfo.push_back(static_cast<PacketInfo>(make_pair(first, second)));
  }
  return;
};

ePacketType getPktType(void *packet)
{
  return *((ePacketType *)packet);
};