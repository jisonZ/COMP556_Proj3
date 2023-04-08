#include "utils.h"

#include <cstring>

ePacketType getPacketType(void *packet) { 
  return (ePacketType)(*((unsigned char *)packet)); 
};

// extract payload from DV packet
// format of packetInfo list:
// line 0: (source router id, destination router id)
// line 1: (node id, cost 1)
// line 2: (node id, cost 2)
// ...
void getDVPacketPayload(void *start, DVL &packetInfo) {
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
    packetInfo.push_back(static_cast<PacketInfo>(make_pair(first, second)));
  }
  return;
};

// extract payload from LS packet (neighbor id, cost)
void getLSPacketPayload(void *packet, pkt_size &size, router_id &srcRouterId, seq_num &sequenceNum,
                        cost_map_entry &costMapEntry) {
  size = (unsigned short)ntohs(*(unsigned short *)((char *)packet + 2));
  srcRouterId = (unsigned short)ntohs(*(unsigned short *)((char *)packet + 4));
  sequenceNum = (unsigned int)ntohl(*(unsigned int *)((char *)packet + 8));

  for (size_t i = 12; i < size; i += 4) {
    router_id neighbor_id = ntohs(*(unsigned short *)((char *)packet + i));
    cost_time cost = ntohs(*(unsigned short *)((char *)packet + i + 2));
    costMapEntry[neighbor_id] = cost;
  }
};