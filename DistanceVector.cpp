#include "DistanceVector.h"

void DistanceVector::init(Node* sys, router_id routerId, unsigned short numPorts, neighbors_pointer neighbors,
    portStatus_pointer portStatus, forwarding_pointer forwarding) {

}

void DistanceVector::recvPacket(port_number port, void* packet, unsigned short size) {
    DVL DVList;
    changeDVPacketToPacketInfo(packet, DVList);
    router_id source_router_id = DVList[0].first;
    

    for (auto it = DVList.begin() + 1; it != DVList.end(); ++it){

    } 
}


void DistanceVector::insertNeighbors(router_id neighborId, port_number port, DVL& DVList) {

}

void DistanceVector::updateDVTable(router_id destId, cost_time cost, router_id next_hop_id) {

}

void DistanceVector::insertDVEntry(router_id destId, cost_time cost, router_id next_hop_id) {

}

void DistanceVector::deleteDVTable(router_id destId) {

}