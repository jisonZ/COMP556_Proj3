#include "utils.h"

#include <cstring>

ePacketType getPacketType(void *packet) { 
  return (ePacketType)(*((unsigned char *)packet)); 
};

void getDVPacketPayload(void *start, DVL &pktInfo) {
  /*
   * start: The DV packet,
   * size : How many bytes of the DV packet
   */
  unsigned short *packet_start = (unsigned short *)start;
  unsigned short size = ntohs(*(packet_start + 1));  // get size of DV packet
  size -= 4;  // delete first row (packet type, reserved, size)
  size /= 4;  // return number of rows of DV payload
  unsigned short first, second;
  size_t i;
  for (i = 0; i < size; ++i) {
    first = ntohs(*((unsigned short *)start + 2 + i * 2));
    second = ntohs(*((unsigned short *)start + 2 + i * 2 + 1));
    // cout <<"[getDVPacketPayload] first: " << first << ", second: " << second << endl;
    pktInfo.push_back(static_cast<PacketInfo>(make_pair(first, second)));
  }
  return;
};

void getLSPacketPayload(void *packet, pkt_size &size, router_id &srcRouterId, seq_num &sequenceNum,
                        cost_map_entry &costMapEntry) {
  size = (unsigned short)ntohs(*(unsigned short *)((char *)packet + 2));
  srcRouterId = (unsigned short)ntohs(*(unsigned short *)((char *)packet + 4));
  sequenceNum = (unsigned int)ntohl(*(unsigned int *)((char *)packet + 8));

  size_t i = 12;
  for (size_t i = 0; i < size; i += 4) {
    router_id neighbor_id = ntohs(*(unsigned short *)((char *)packet + i));
    cost_time cost = ntohs(*(unsigned short *)((char *)packet + i + 2));
    costMapEntry[neighbor_id] = cost;
  }
};