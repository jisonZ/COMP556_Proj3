#ifndef DISTANCEVECTOR_H
#define DISTANCEVECTOR_H

#include "Node.h"
#include "utils.h"

// dest_ID -> [next_hop_id, cost, last_update_time]
using DVTable_pointer = unordered_map<router_id, DVEntry> *;

class DistanceVector
{
public:
    void init(Node *sys, router_id routerId, unsigned short numPorts, neighbors_pointer neighbors,
              portStatus_pointer portStatus, forwarding_pointer forwarding);

    void recvPacket(port_number port, void *packet, unsigned short size);
    void insertNeighbors(router_id neighborId, port_number port, DVL &DVList);
    void updateDVTable(router_id destId, cost_time cost, router_id next_hop_id);
    void insertDVEntry(router_id destId, cost_time cost, router_id next_hop_id);
    void deleteDVEntry(router_id destId);
    void sendPacket(DVL &DVList);
    void sendPacket();
    void checkLink();
    void removeInvalidDVEntry(DVL &DVList, router_id disconnectedNeighbor);
    
    void printDVTable();

    friend class RoutingProtocolImpl;
    DVTable_pointer DVTable;

private:
    Node *sys;
    router_id routerId;
    unsigned short numOfPorts;
    neighbors_pointer neighbors;        // pointer reference, initialized by RoutingProtodcolImpl.cc
    portStatus_pointer portStatus;      // ...
    forwarding_pointer forwardingTable; // ...
};

#endif