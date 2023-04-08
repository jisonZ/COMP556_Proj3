#ifndef LINKSTATE_H
#define LINKSTATE_H

#include <queue>

#include "Node.h"
#include "utils.h"

using seqNum_pointer = unordered_map<router_id, seq_num> *;

class LinkState {
 public:
  void init(Node *sys, router_id id, port_number num_ports, portStatus_pointer portStatus,
            forwarding_pointer fwtp);
  // forward data to next hop router according to entry in forwarding table
  void sendData(router_id destRouterId, pkt_size size, port_number port, void *packet);
  // if Pong received, send LSP to update if: 1) new neighbor found 2) changes occur
  void recvPong(port_number port, router_id neighborId, cost_time RTT);
  // recv newer LSP and flood to all neighbors
  void recvLSP(port_number port, void *packet);
  // assemble LSP and send when: 1) periodically 2) on-demand when link state changes
  void sendLSP();
  // main logic for LS: dijkstra's algorithm to calculate shortest path
  void updateLSTable();
  // check and remove LS entries in LSTable which are older than 45 seconds
  bool isExpiredLSEntryRemoved();
  // check if link is expired (over 15 seconds) and should be declared dead
  bool checkLink();

 private:
  Node *sys;
  router_id routerID;
  unsigned short numPorts;
  // port_number -> [to_router_id, cost, last_update_time, is_connect]
  portStatus_pointer portStatus;
  // dest_router_id -> next_hop_router_id
  forwarding_pointer forwardingTable;
  // sequence number of LSP
  seq_num sequenceNum = 0;


  // For Dijkstra

  // router id -> [cost, previous router id, last update time]
  unordered_map<router_id, LSEntry> LSTable;

  // router id -> [neighbor router id -> cost]
  // populated by (neighbor id, cost) info from Ping-Pong; compared to adjacency list
  unordered_map<router_id, cost_map_entry> cost_map;

  // router id -> sequence number
  // contains sequence number for all the other routers in network
  unordered_map<router_id, seq_num> seq_num_map;


  // LS Helper Functions:

  // LS flooding
  void floodLSP(port_number fromPort, void *packet, pkt_size size);
  // get router_id of the next hop router for a given dest_router
  router_id getNextHop(unordered_map<router_id, pair<cost_time, router_id>> distances,
                       router_id dest_router);
  // check if [neighbor id, cost] info in received LSP is different from what's in cost_map
  bool hasCostMapEntryChanged(router_id neighbor_id, cost_map_entry costMapEntry);
  void printLSForwardingTable();
};

#endif