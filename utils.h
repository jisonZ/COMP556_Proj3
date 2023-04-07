#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include "global.h"
using std::pair;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using std::unordered_set;
using std::unordered_map;
using std::set;
using port_number = unsigned short;
using router_id = unsigned short;
using cost_time = unsigned short;
using time_stamp = unsigned int;
using seq_num = unsigned int;

using PacketInfo = pair<unsigned short, unsigned short>;

using DVL = vector<PacketInfo>;
using LSL = vector<PacketInfo>;

enum eAlarmType
{
  SendPINGPONG,
  SendDV,
  SendLS,
  SendCheck,
};

struct Neighbor
{
  port_number port;
  cost_time cost;
  Neighbor() {}
  Neighbor(port_number port, cost_time cost) : port(port), cost(cost) {}
};

struct PortEntry
{
  router_id to_router_id;
  cost_time cost;
  time_stamp last_update_time;
  bool is_connected;
};

struct DVEntry
{
  router_id next_hop_id;
  cost_time cost;
  time_stamp last_update_time;
  DVEntry() {}
  DVEntry(router_id next_hop_id, cost_time cost, time_stamp last_update_time) : next_hop_id(next_hop_id), cost(cost), last_update_time(last_update_time) {}
};

// neighbor_ID -> [neighbor_port, neighbor_cost]
using neighbors_pointer = unordered_map<router_id, Neighbor> *;
// port_number -> [to_router_id, cost, last_update_time, is_connect]
using portStatus_pointer = unordered_map<port_number, PortEntry> *;
// dest_ID -> next_hop_id
using forwarding_pointer = unordered_map<router_id, router_id> *;

void changeDVPacketToPacketInfo(void *start, vector<PacketInfo> &pktInfo);
bool getLSinfo(void *packet, unordered_map<router_id, cost_time> &LSlist, seq_num &seqNum, router_id& routerID);
ePacketType getPktType(void *packet);
void testPktType(ePacketType t);
#endif