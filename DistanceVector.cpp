#include "DistanceVector.h"

void DistanceVector::init(Node* sys, router_id routerId, unsigned short numPorts, neighbors_pointer neighbors,
    portStatus_pointer portStatus, forwarding_pointer forwarding) {
    this->sys = sys;
    this->routerId = routerId;
    this->numOfPorts = numPorts;
    this->neighbors = neighbors;
    this->portStatus = portStatus;
    this->forwardingTable = forwarding;
    this->DVTable = new unordered_map<router_id, DVEntry>();
}

void DistanceVector::recvPacket(port_number port, void* packet, unsigned short size) {
    DVL DVList;
    changeDVPacketToPacketInfo(packet, DVList);
    router_id source_router_id = DVList[0].first;
    // insertNeighbors(source_router_id, port, DVList);
    auto source_iterator = DVTable->find(source_router_id);
    
    if (source_iterator == DVTable->end()){
        cerr << "neighbor cost not detected" << endl;
        return;
    }
    
    cost_time source_cost = (source_iterator->second).cost;
    DVL new_packet_list {}; 
    //Update the neighbors

    for (auto it = DVList.begin() + 1; it != DVList.end(); ++it){
        router_id target_router_id = it->first;
        cost_time recv_cost = it->second;
       
        auto cit = DVTable->find(target_router_id);
        

        if (recv_cost == INFINITY_COST ){
            // Poison Reverse
            if (cit == DVTable->end()) continue;
            if (cit->second.next_hop_id != source_router_id) continue;
            if (neighbors->find(target_router_id) != neighbors->end()){
                // Direct Neighbor, ignore, set cost to new cost (updated by Ping Pong Event)
                auto new_cost = neighbors->find(target_router_id)->second.cost;
                updateDVTable(target_router_id, new_cost, target_router_id);
                new_packet_list.emplace_back(target_router_id, new_cost);
            }
            else deleteDVEntry(source_router_id); // Otherwise, delete
            continue;
        }
        if (source_router_id != cit->second.next_hop_id){
            continue;
        }
        auto target_cost = (cit->second).cost;
        
        if (neighbors->find(target_router_id) != neighbors->end() && target_cost <= recv_cost + source_cost){
            auto new_cost = neighbors->find(target_router_id)->second.cost;
            updateDVTable(target_router_id, new_cost, target_router_id);
            new_packet_list.emplace_back(target_router_id, new_cost);
            continue;
        }

        if (cit == DVTable->end()){ 
            // New Entry
            insertDVEntry(target_router_id, recv_cost + source_cost, source_router_id);
            new_packet_list.emplace_back(target_router_id, recv_cost + source_cost);
            
        } 
        
        else {
            // Update Entry
            if (target_cost > source_cost + recv_cost){ 
                updateDVTable(target_router_id, source_cost + recv_cost, source_router_id);
                new_packet_list.emplace_back(target_router_id, recv_cost + source_cost);
            }
        }
    } 
    if (!new_packet_list.empty()) sendPacket(new_packet_list);
}


// void DistanceVector::insertNeighbors(router_id neighborId, port_number port, DVL& DVList) {
//     if ((*neighbors).count(neighborId) == 0) {
        
//     }
// }

void DistanceVector::updateDVTable(router_id destId, cost_time cost, router_id next_hop_id) {
    DVEntry de = (*DVTable)[destId];
    de.next_hop_id = next_hop_id;
    de.cost = cost;
    de.last_update_time = sys->time();
    (*forwardingTable)[destId] = next_hop_id;
}

void DistanceVector::insertDVEntry(router_id destId, cost_time cost, router_id next_hop_id) {
    DVEntry de {next_hop_id, cost, sys->time()};
    (*DVTable)[destId] = de;
    (*forwardingTable)[destId] = next_hop_id;
}

void DistanceVector::deleteDVEntry(router_id destId) {
    DVTable->erase(destId);
}

void DistanceVector::sendPacket(DVL& DVList){
    // Based on port to send packet
    unsigned int size = DVList.size() * 4 + 8;
    char* msg = new char[size];
    *msg = (unsigned char)DV;
    auto packet = (unsigned short *)msg;
    *(packet + 1) = htons(size);
    *(packet + 2) = htons(routerId);

    for (port_number port_id = 0; port_id < numOfPorts; ++ port_id){
        auto pit = portStatus->find(port_id);
        if (!(pit->second.is_connected)) continue;
        uint32_t index = 0;
        for (auto & dv : DVList){
            auto target_router_id = dv.first, cost = dv.second;
            if (neighbors->find(target_router_id) != neighbors->end() && cost > 1.5*neighbors->find(target_router_id)->second.cost)
                cost = INFINITY_COST; // Poison Recover
            *(packet + 4 + index++) = htons(target_router_id);
            *(packet + 4 + index++) = htons(cost);
        }
        *(packet + 3) = htons(pit->second.to_router_id);
        sys->send(port_id, msg, size);
    }
    delete[] msg;
}

void sendPacket(){
    DVL DVList {};
    for (auto it =  DVTable->first() ; it != DVTable->end(); ++it)
        DVList.push_back({it->first, it->second.cost});
    sendPacket(DVList);
}

