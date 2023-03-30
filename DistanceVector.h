#ifndef DISTANCEVECTOR_H
#define DISTANCEVECTOR_H

#include "Node.h"
#include "utils.h"

using neighbors_pointer = unordered_map<router_id, Neighbor> *;
using portStatus_pointer = unordered_map<port_number, PortEntry>*;
using forwarding_pointer = unordered_map<router_id, router_id>*;
using DVTable_pointer =unordered_map<router_id, DVEntry>*;


class DistanceVector {
public:
    void init(Node* sys, router_id routerId, unsigned short numPorts, neighbors_pointer neighbors,
    portStatus_pointer portStatus, forwarding_pointer forwarding);

    void recvPacket(port_number port, void* packet, unsigned short size);

    void insertNeighbors(router_id neighborId, port_number port, DVL& DVList);

    void updateDVTable(router_id destId, cost_time cost, router_id next_hop_id);

    void insertDVEntry(router_id destId, cost_time cost, router_id next_hop_id);

    void deleteDVTable(router_id destId);

private:
    Node *sys;
    router_id routerId;
    unsigned short numOfPorts;
    neighbors_pointer neighbors; // pointer reference, initialized by RoutingProtodcolImpl.cc
    portStatus_pointer portStatus;  // ...
    forwarding_pointer fowardingTable; // ...
    DVTable_pointer DVTable; // new
};

#endif