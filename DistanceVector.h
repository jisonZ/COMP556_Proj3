#ifndef DISTANCEVECTOR_H
#define DISTANCEVECTOR_H

#include "Node.h"
#include "utils.h"

class DistanceVector {
 public:
  void init(Node *sys, router_id routerID, unsigned short num_ports, neighbors_pointer neighbors,
            portStatus_pointer portStatus, forwarding_pointer forwarding);

  // forward data to next hop router according to entry in forwarding table
  void sendData(router_id destRouterId, pkt_size size, port_number port, void *packet);
  // if Pong received, send DV message conditionally
  void recvPong(port_number port, router_id neighborId, cost_time RTT, bool isConnected);
  // main logic of DV: exchange DV information with neighbors
  void recvPacket(port_number port, void *packet, unsigned short size);
  // assemble ans send DV message
  void sendPacket();
  // check whether all links are up (no older than 15 seconds), remove invalid entries and send updates
  void checkLink();

 private:
  Node *sys;
  router_id routerID;
  unsigned short numPorts;
  // neighbor_ID -> [neighbor_port, neighbor_cost]
  neighbors_pointer neighbors;
  // port_number -> [to_router_id, cost, last_update_time, is_connect]
  portStatus_pointer portStatus;
  // dest_router_id -> next_hop_router_id
  forwarding_pointer forwardingTable;
  // dest_ID -> [next_hop_id, cost, last_update_time]
  DVTable_pointer DVTable;


  // DV Helper Functions:

  void insertNeighbors(router_id neighborId, port_number port, DVL &DVList);
  void updateDVTable(router_id destId, cost_time cost, router_id next_hop_id);
  void insertDVEntry(router_id destId, cost_time cost, router_id next_hop_id);
  void deleteDVEntry(router_id destId);
  void sendPacket(DVL &DVList);
  void removeInvalidDVEntry(DVL &DVList, router_id disconnectedNeighbor);
  void printDVTable();
};

#endif