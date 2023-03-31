#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>

#include "global.h"
using std::vector;
using std::pair;
using port_number = unsigned short;
using router_id = unsigned short;
using cost_time = unsigned short;
using time_stamp = unsigned int;
using PacketInfo = pair<unsigned short, unsigned short>;
using DVL = vector<PacketInfo>; 

enum eAlarmType {
  SendPINGPONG,
  SendDV,
  SendLS,
  SendExpire,
};


struct Neighbor {
  port_number port;
  cost_time cost;
  Neighbor () {}
  Neighbor (port_number port, cost_time cost) : port(port), cost(cost) {}
};

struct PortEntry {
  router_id to_router_id;
  cost_time cost;
  time_stamp last_update_time;
  bool is_connected;
};

struct DVEntry{
  router_id next_hop_id;
  cost_time cost;
  time_stamp last_update_time;
  DVEntry() {}
  DVEntry(router_id next_hop_id, cost_time cost, time_stamp last_update_time): next_hop_id(next_hop_id), cost(cost), \ 
  last_update_time(last_update_time) {}
};

// struct PacketInfo {
//   router_id router;
//   cost_time cost;
// };

void changeDVPacketToPacketInfo(void* start, vector<PacketInfo>& pktInfo){
  /*
  * start: The DV packet, 
  * size : How many bytes of the DV packet
  */
  unsigned short* packet_start = (unsigned short*)start; // 
  unsigned short size = ntohs(*(packet_start + 1)); // Get Size of DV packet
  size -= 4; // Delete First Row (Packet Type, Reserved, Size)
  size /= 4; // Return How Many Rows of DV data
  unsigned short first, second;
  size_t i;
  for (i = 0; i < size; ++i){
    first = ntohs(*((unsigned short*) start + i * 2));
    second = ntohs(*((unsigned short*) start + i * 2 + 1));
    pktInfo.push_back(static_cast<PacketInfo>(make_pair(first, second)));
  }
  return;
};

ePacketType getPktType(void* packet) {
    return *((ePacketType*)packet);
};

#endif