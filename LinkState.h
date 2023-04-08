#ifndef LINKSTATE_H
#define LINKSTATE_H

#include <queue>

#include "Node.h"
#include "utils.h"

using seqNum_pointer = unordered_map<router_id, int> *;

class LinkState {
 public:
  void init(Node *sys, router_id id, port_number num_ports, portStatus_pointer portStatus,
            forwarding_pointer fwtp);
  void floodLSP(port_number fromPort, void *packet, pkt_size size);
  void recvLSP(port_number port, void *packet, pkt_size size);
  void sendLSP();
  void updateLSTable();
  bool isExpiredLSEntryRemoved();

 private:
  Node *sys;
  router_id routerID;
  unsigned short numPorts;
  portStatus_pointer portStatus;
  forwarding_pointer forwardingTable;
  seq_num sequenceNum = 0;  // sequence number

  // For Dijkstra

  // router id -> [cost, previous router id, last update time]
  unordered_map<router_id, LSEntry> LSTable;

  // router id -> [neighbor router id -> cost]
  // populated by (neighbor id, cost) info from Ping-Pong
  unordered_map<router_id, cost_map_entry> cost_map;

  // router id -> sequence number
  // contains sequence number for all the other routers in network
  unordered_map<router_id, seq_num> seq_num_map;

  // helper functions for LS

  router_id getNextHop(unordered_map<router_id, pair<cost_time, router_id>> distances,
                       router_id dest_router);
  bool hasCostMapChanged(router_id neighbor_id,
                         unordered_map<router_id, cost_time> neighbor_id_to_cost);
};

#endif