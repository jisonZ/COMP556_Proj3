#include "DistanceVector.h"

#include <cstring>

void DistanceVector::init(Node *sys, router_id routerID, unsigned short numPorts,
                          neighbors_pointer neighbors, portStatus_pointer portStatus,
                          forwarding_pointer forwarding) {
  // initialize tables and neighbor information
  this->sys = sys;
  this->routerID = routerID;
  this->numPorts = numPorts;
  this->neighbors = neighbors;
  this->portStatus = portStatus;
  this->forwardingTable = forwarding;
  this->DVTable = new unordered_map<router_id, DVEntry>();
}

// forward data to next hop router according to entry in forwarding table 
void DistanceVector::sendData(router_id destRouterId, pkt_size size, port_number port, void *packet) {
  // drop DATA packet sending back to self
  if (destRouterId == routerID) {
    free(packet);
    return;
  }

  // drop DATA packet if no entry in forwardingTable
  if (forwardingTable->find(destRouterId) == forwardingTable->end()) {
    return;
  }
  router_id nextHopRouterId = (*forwardingTable)[destRouterId];

  // drop DATA packet if next hop not in neighbors table
  if (neighbors->find(nextHopRouterId) == neighbors->end()) {
    return;
  }

  sys->send((*neighbors)[nextHopRouterId].port, packet, size);
}

// if Pong received, send DV message when: 
// 1) new neighbor detected or disconnected neighbor reconnecting
// 2) local link cost change -> need to notify all affected neighbors 
void DistanceVector::recvPong(port_number port, router_id neighborId, cost_time RTT,
                              bool isConnected) {
  // check if this neighbor is already connected before
  if (neighbors->find(neighborId) != neighbors->end() && isConnected) {
    // update connected neighbor's cost and port number
    (*neighbors)[neighborId].port = port;
    cost_time oldRTT = (*neighbors)[neighborId].cost;
    (*neighbors)[neighborId].cost = RTT;
    cost_time rttDiff = RTT - oldRTT;

    // cost has changed
    if (rttDiff != 0) {
      for (auto it = DVTable->begin(); it != DVTable->end(); ++it) {
        it->second.last_update_time = sys->time();

        // router_id of current entry in DVTable
        router_id currRouterId = it->first;

        // current router use neighborId as next hop
        if (it->second.next_hop_id == neighborId) {
          cost_time newRTT = it->second.cost + rttDiff;

          // going to next_hop router is more expensive now -> go directly from currRouterId instead
          if (neighbors->find(currRouterId) != neighbors->end() && (*neighbors)[currRouterId].cost < newRTT) {
            it->second.next_hop_id = currRouterId;
            it->second.cost = (*neighbors)[currRouterId].cost;
            (*forwardingTable)[currRouterId] = currRouterId;
          } else {  // update cost
            it->second.cost = newRTT;
          }
        }
        // current router is a direct neighbor, and now it has a new min cost
        else if (currRouterId == neighborId && (*DVTable)[neighborId].cost > RTT) {
          it->second.next_hop_id = neighborId;
          it->second.cost = RTT;
          (*forwardingTable)[neighborId] = neighborId;
        }
      }
      sendPacket();
    }
    // cost has not changed, just update the timestamp for each entry
    else {
      for (auto it = DVTable->begin(); it != DVTable->end(); ++it) {
        if (it->second.next_hop_id == neighborId || it->first == neighborId) {
          it->second.last_update_time = sys->time();
        }
      }
    }
  }
  // neighbor is either new or re-connecting
  else {
    // re-connecting: DVTable has an entry for neighborId but port is not connected
    if (DVTable->find(neighborId) != DVTable->end() && !isConnected) {
      // TODO: do we need if-else here?
      if ((*DVTable)[neighborId].cost > RTT) {
        updateDVTable(neighborId, RTT, neighborId);
        sendPacket();
      } else {
        (*DVTable)[neighborId].last_update_time = sys->time();
      }
      // new neighbor
    } else {
      insertDVEntry(neighborId, RTT, neighborId);
      sendPacket();
    }
    (*forwardingTable)[neighborId] = neighborId;
  }
}

// main logic of DV: exchange DV information with neighbors
void DistanceVector::recvPacket(port_number port, void *packet, unsigned short size) {
  DVL DVList;

  // extract source id and (neighbor, cost) information from packet
  getDVPacketPayload(packet, DVList);

  // if receive empty info, exit
  if (DVList.size() == 0) return;

  router_id source_router_id = DVList[0].first;

  // insert neighbor and cost if not exist
  insertNeighbors(source_router_id, port, DVList);

  // if cannot find neighbor, exit
  if (neighbors->find(source_router_id) == neighbors->end()) {
    // cout << "cannot find neighbor, exit" << endl;
    return;
  }

  // update neighbor DV table entry and forwarding table entry
  cost_time source_cost = neighbors->find(source_router_id)->second.cost;

  insertDVEntry(source_router_id, source_cost, source_router_id);

  DVL new_packet_list{};

  // Update the neighbors
  for (auto it = DVList.begin() + 1; it != DVList.end(); ++it) {
    router_id target_router_id = it->first;
    cost_time recv_cost = it->second;

    auto cit = DVTable->find(target_router_id);

    if (cit == DVTable->end()) {
      // suppose we do not have a path to target_router_id
      // TODO: is the second condition check necessary?
      if (recv_cost == INFINITY_COST || target_router_id == routerID) continue;
      insertDVEntry(target_router_id, recv_cost + source_cost, source_router_id);
      new_packet_list.emplace_back(target_router_id, recv_cost + source_cost);
      continue;
    }

    if (recv_cost == INFINITY_COST) {
      // Poison Reverse
      if (cit == DVTable->end()) continue;
      if (cit->second.next_hop_id != source_router_id) continue;
      if (neighbors->find(target_router_id) != neighbors->end()) {
        // Direct Neighbor, ignore, set cost to new cost (updated by Ping Pong Event)
        auto new_cost = neighbors->find(target_router_id)->second.cost;
        updateDVTable(target_router_id, new_cost, target_router_id);
        new_packet_list.emplace_back(target_router_id, new_cost);
      } else
        deleteDVEntry(source_router_id);  // Otherwise, delete
      continue;
    }
    if (source_router_id != cit->second.next_hop_id) {
      continue;
    }
    auto target_cost = (cit->second).cost;

    if (neighbors->find(target_router_id) != neighbors->end() &&
        target_cost <= recv_cost + source_cost) {
      auto new_cost = neighbors->find(target_router_id)->second.cost;
      updateDVTable(target_router_id, new_cost, target_router_id);
      new_packet_list.emplace_back(target_router_id, new_cost);
      continue;
    }

    else {
      // Update Entry
      if (target_cost > source_cost + recv_cost) {
        updateDVTable(target_router_id, source_cost + recv_cost, source_router_id);
        new_packet_list.emplace_back(target_router_id, recv_cost + source_cost);
      }
    }
  }

  printDVTable();

  if (!new_packet_list.empty()) {
    sendPacket(new_packet_list);
  }
  // delete[] (char *)packet;
  free(packet);
}

void DistanceVector::insertNeighbors(router_id neighborId, port_number port, DVL &DVList) {
  // if neighbor already exist in neighbors list, exit
  if (neighbors->find(neighborId) != neighbors->end()) return;

  // search for self routerID and cost
  for (auto it = DVList.begin() + 1; it != DVList.end(); it++) {
    auto n = *it;
    if (n.first == routerID) {
      Neighbor newNeighbor(port, n.second);
      (*neighbors)[neighborId] = newNeighbor;
      break;
    }
  }
}

void DistanceVector::updateDVTable(router_id destId, cost_time cost, router_id next_hop_id) {
  auto de = DVTable->find(destId);
  if (de == DVTable->end()) {
    return;
  }
  de->second.next_hop_id = next_hop_id;
  de->second.cost = cost;
  de->second.last_update_time = sys->time();
  (*forwardingTable)[destId] = next_hop_id;
}

void DistanceVector::insertDVEntry(router_id destId, cost_time cost, router_id next_hop_id) {
  DVEntry de(next_hop_id, cost, sys->time());

  (*DVTable)[destId] = de;
  (*forwardingTable)[destId] = next_hop_id;
}

void DistanceVector::deleteDVEntry(router_id destId) {
  if (DVTable->find(destId) != DVTable->end()) DVTable->erase(destId);
}

void DistanceVector::sendPacket(DVL &DVList) {
  unsigned int size = DVList.size() * 4 + 8;

  for (port_number port_id = 0; port_id < numPorts; ++port_id) {
    // continue if port is detached
    auto pit = portStatus->find(port_id);
    if (pit == portStatus->end() || !(pit->second.is_connected)) continue;

    char *msg = new char[size];
    *msg = (unsigned char)DV;
    auto packet = (unsigned short *)msg;

    *(packet + 1) = htons(size);
    *(packet + 2) = htons(routerID);
    *(packet + 3) = htons(pit->second.to_router_id);

    // iterate DV List
    uint32_t index = 0;

    for (auto &dv : DVList) {
      auto target_router_id = dv.first, cost = dv.second;
      auto next_hop_id = (*DVTable)[target_router_id].next_hop_id;

      if (neighbors->find(next_hop_id) != neighbors->end() &&
          neighbors->find(next_hop_id)->second.port == port_id) {
        cost = INFINITY_COST;
      }

      *(packet + 4 + index++) = htons(target_router_id);
      *(packet + 4 + index++) = htons(cost);
    }

    // printDVTable();

    // TODO: do we need memcpy?
    sys->send(port_id, msg, size);
  }
}

// assemble ans send DV message when: 1) periodically 2) on-demanc when DV info changes
void DistanceVector::sendPacket() {
  DVL DVList{};
  for (auto it = DVTable->begin(); it != DVTable->end(); ++it) {
    DVList.emplace_back(it->first, it->second.cost);
  }

  sendPacket(DVList);
}

// check whether neighbors are still connected, remove invalid entries and send updates
void DistanceVector::checkLink() {
  auto pairs = DVL{};
  for (auto &port : *portStatus) {
    if (!port.second.is_connected || sys->time() - port.second.last_update_time <= 15 * 1000) {
      continue;
    }
    port.second.cost = 0;
    port.second.is_connected = false;
    auto disconnectedNeighbor = port.second.to_router_id;
    if (neighbors->find(disconnectedNeighbor) != neighbors->end()) {
      neighbors->erase(disconnectedNeighbor);
    }
    removeInvalidDVEntry(pairs, disconnectedNeighbor);
  }
  if (!pairs.empty()) {
    sendPacket(pairs);
  }
}

void DistanceVector::removeInvalidDVEntry(DVL &DVList, router_id disconnectedNeighbor) {
  unordered_set<unsigned short>
      invalid_entries;  // store router id's of invalid entries to be removed
  for (auto &it : *DVTable) {
    if (it.second.next_hop_id != disconnectedNeighbor &&
        sys->time() - it.second.last_update_time <= 45 * 1000) {
      continue;
    }
    if (neighbors->find(it.first) == neighbors->end()) {
      // only remove those node that aren't neighbors
      invalid_entries.emplace(it.first);
      continue;
    }
    if (neighbors->find(it.first)->second.cost != it.second.cost) {
      updateDVTable(it.first, neighbors->find(it.first)->second.cost, it.first);
      DVList.emplace_back(it.first, it.second.cost);
    }
  }
  for (auto &id : invalid_entries) {
    deleteDVEntry(id);
  }
}

void DistanceVector::printDVTable() {
  cout << "------------------DV Table------------------" << endl;
  cout << "DVTable size: " << DVTable->size() << endl;
  for (auto it = DVTable->begin(); it != DVTable->end(); ++it) {
    cout << "router_id: " << it->first << " | cost: " << it->second.cost
         << " | next_hop_id: " << it->second.next_hop_id
         << " | timestamp: " << it->second.last_update_time << endl;
  }
  cout << "--------------------End---------------------" << endl;
}