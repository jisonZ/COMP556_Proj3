#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>

#include <set>
#include <unordered_map>
#include <unordered_set>

#include "global.h"
using std::cerr;
using std::cout;
using std::endl;
using std::pair;
using std::set;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using port_number = unsigned short;
using router_id = unsigned short;
using cost_time = unsigned short;
using time_stamp = unsigned int;
using seq_num = unsigned int;
using pkt_size = unsigned short;

using PacketInfo = pair<unsigned short, unsigned short>;
using cost_map_entry = unordered_map<router_id, cost_time>;

using DVL = vector<PacketInfo>;

enum eAlarmType {
  SendPINGPONG,
  SendDV,
  SendLS,
  SendCheck,
};

struct Neighbor {
  port_number port;
  cost_time cost;
  Neighbor() {}
  Neighbor(port_number port, cost_time cost) : port(port), cost(cost) {}
};

struct PortEntry {
  router_id to_router_id;
  cost_time cost;
  time_stamp last_update_time;
  bool is_connected;
};

struct DVEntry {
  router_id next_hop_id;
  cost_time cost;
  time_stamp last_update_time;
  DVEntry() {}
  DVEntry(router_id next_hop_id, cost_time cost, time_stamp last_update_time)
      : next_hop_id(next_hop_id), cost(cost), last_update_time(last_update_time) {}
};

struct LSEntry {
  unsigned int cost;
  router_id prev_router_id;
  time_stamp last_update_time;
};

// neighbor_ID -> [neighbor_port, neighbor_cost]
using neighbors_pointer = unordered_map<router_id, Neighbor> *;
// port_number -> [to_router_id, cost, last_update_time, is_connect]
using portStatus_pointer = unordered_map<port_number, PortEntry> *;
// dest_router_id -> next_hop_router_id
using forwarding_pointer = unordered_map<router_id, router_id> *;
// dest_ID -> [next_hop_id, cost, last_update_time]
using DVTable_pointer = unordered_map<router_id, DVEntry> *;

ePacketType getPacketType(void *packet);
void getDVPacketPayload(void *start, DVL &packetInfo);
void getLSPacketPayload(void *packet, pkt_size &size, router_id &srcRouterId, 
                        seq_num &sequenceNum, cost_map_entry &costMapEntry);

#endif