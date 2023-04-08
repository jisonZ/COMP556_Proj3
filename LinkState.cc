#include "LinkState.h"

#include <cstring>

void LinkState::init(Node *sys, router_id id, port_number numPorts, portStatus_pointer ports,
                     forwarding_pointer fwtp) {
  this->sys = sys;
  this->routerID = id;
  this->numPorts = numPorts;
  this->portStatus = ports;
  this->forwardingTable = fwtp;
}

void LinkState::recvLSP(port_number port, void *packet, pkt_size size) {
  pkt_size size;
  router_id sourceRouterId;
  seq_num receivedSeqNum;
  cost_map_entry costMapEntry;

  getLSPacketPayload(packet, size, sourceRouterId, receivedSeqNum, costMapEntry);

  if (seq_num_map.find(sourceRouterId) == seq_num_map.end() ||
      seq_num_map[sourceRouterId] < receivedSeqNum) {
    seq_num_map[sourceRouterId] = receivedSeqNum;

    if (hasCostMapChanged(sourceRouterId, costMapEntry)) {
      cost_map[sourceRouterId] = costMapEntry;
      updateLSTable();
    }

    floodLSP(port, packet, size);
  }
  free(packet);
}

void LinkState::floodLSP(port_number fromPort, void *packet, pkt_size size) {
  for (port_number portNum = 0; portNum < numPorts; ++portNum) {
    // avoid flooding LSP to incoming port
    if (portNum == fromPort) continue;
    // char *new_data = strdup(static_cast<char *>(packet));
    // sys->send(portNum, new_data, size);
    char *floodingPacket = (char *)malloc(size);
    memcpy(floodingPacket, packet, size);
    sys->send(portNum, (void *)floodingPacket, size);
  }
}

void LinkState::sendLSP() {
  unordered_map<router_id, cost_time> neighborIdToCost;
  for (auto it = portStatus->begin(); it != portStatus->end(); ++it) {
    if (it->second.cost < INFINITY_COST) {
      neighborIdToCost[it->second.to_router_id] = it->second.cost;
    }
  }
  pkt_size size = neighborIdToCost.size() * 4 + 12;

  for (auto it = portStatus->begin(); it != portStatus->end(); ++it) {
    char *packet = (char *)malloc(size);
    *packet = (char)LS;                                                 // packet type
    *(unsigned short *)(packet + 2) = (unsigned short)htons(size);      // packet size
    *(unsigned short *)(packet + 4) = (unsigned short)htons(routerID);  // source ID
    *(unsigned int *)(packet + 8) = (unsigned int)(sequenceNum);        // sequence number
    size_t i = 12;
    for (auto it = neighborIdToCost.begin(); it != neighborIdToCost.end(); ++it) {
      *(unsigned short *)(packet + i) = (unsigned short)htons(it->first);
      *(unsigned short *)(packet + i + 2) = (unsigned short)htons(it->second);
      i += 4;
    }
    sys->send(it->first, packet, size);
  }
  ++sequenceNum;
}

void LinkState::updateLSTable() {
  // <node, <cost, predecessor>>
  unordered_map<router_id, pair<cost_time, router_id>> distances;
  unordered_set<router_id> unvisited_routers;
  // for (auto it = cost_map[1].begin(); it != cost_map[1].end(); ++it)
  // {
  //     cout << it->first<< " " << it->second << endl;
  // }
  // cout << "End Cost Map" << endl;

  // Add adjacent routers to root router to D
  for (auto it = portStatus->begin(); it != portStatus->end(); ++it) {
    PortEntry portEntry = it->second;
    distances[portEntry.to_router_id] = make_pair(portEntry.cost, routerID);
  }

  // Add not directly connected routers to D
  for (auto it = cost_map.begin(); it != cost_map.end(); ++it) {
    router_id neighborId = it->first;
    if (neighborId == routerID) continue;

    unvisited_routers.insert(neighborId);
    if (distances.find(neighborId) == distances.end())  // Not connected to root router
      distances[neighborId] = make_pair(INFINITY_COST, routerID);
  }

  // for (auto it = distances.begin(); it != distances.end(); ++it)
  // {
  //     cout << it->first << " < " << it->second.first << ", " << it->second.second << " > " <<
  //     endl;
  // }

  // cout << "----End distances -----" << endl;
  // for (auto it = unvisited_routers.begin(); it != unvisited_routers.end(); ++it)
  // {
  //     cout << *it << endl;
  // }

  // cout << "------End Unv------" << endl;
  router_id cur_router_id;
  cost_time min_cost;

  // Dijkstra
  while (!unvisited_routers.empty()) {
    // cout << "---Begin---" << endl;
    // Find the shortest router to root router
    min_cost = INFINITY_COST;
    for (auto it = unvisited_routers.begin(); it != unvisited_routers.end(); ++it) {
      router_id unvisited = *it;
      cost_time currCost = distances[unvisited].first;
      if (min_cost > currCost) {
        cur_router_id = unvisited;
        min_cost = currCost;
      }
    }

    // There is no conneceted components to root
    if (min_cost == INFINITY_COST) {
      break;
    }

    // Remove
    unvisited_routers.erase(cur_router_id);

    // Update Connection if it is shorter than cache
    auto costMapEntry = cost_map[cur_router_id];
    for (auto it = costMapEntry.begin(); it != costMapEntry.end(); ++it) {
      router_id neighbor_id = it->first;
      cost_time cost = it->second;
      if (unvisited_routers.find(neighbor_id) != unvisited_routers.end())
        if (distances[neighbor_id].first > cost + min_cost)
          distances[neighbor_id] = make_pair(cost + min_cost, cur_router_id);
    }
  }

  // Update Node Info and Forwarding Table
  for (auto it = distances.begin(); it != distances.end(); ++it) {
    router_id cur_id = it->first;
    pair<cost_time, router_id> d = it->second;
    if (d.first == INFINITY_COST) continue;
    LSEntry lsEntry;
    lsEntry.cost = d.first;
    lsEntry.prev_router_id = d.second;
    lsEntry.last_update_time = sys->time();
    LSTable[cur_id] = lsEntry;
    (*forwardingTable)[cur_id] = getNextHop(distances, cur_id);
  }
  // cout << "LS" << endl;
}

router_id LinkState::getNextHop(unordered_map<router_id, pair<cost_time, router_id>> distances,
                                router_id dest_router) {
  router_id cur_router_id = dest_router;
  while (distances[cur_router_id].second != routerID)
    cur_router_id = distances[cur_router_id].second;
  return cur_router_id;
}

// check and remove LS entries in LSTable which are older than 45 seconds
bool LinkState::isExpiredLSEntryRemoved() {
  unsigned int now;
  vector<router_id> expired;  // store router id of expired LS entries

  for (auto it : this->LSTable) {
    now = sys->time();
    if (now - it.second.last_update_time >= 45 * 1000) {
      expired.push_back(it.first);
    }
  }

  for (router_id routerId : expired) {
    this->LSTable.erase(routerId);
  }

  return !expired.empty();
}

// check if [neighbor id, cost] info in received LSP packet is different from cost_map
bool LinkState::hasCostMapChanged(router_id neighbor_id,
                                  unordered_map<router_id, cost_time> neighbor_id_to_cost) {
  // cost_map does not contain info from this neighbor yet
  if (this->cost_map.find(neighbor_id) == this->cost_map.end()) {
    return !neighbor_id_to_cost.empty();
  } else {
    //                         cost_map_entry
    // cost_map: [router_id -> [neighbor_router_id -> cost]]
    cost_map_entry currCostMapEntry = this->cost_map[neighbor_id];
    if (currCostMapEntry.size() != neighbor_id_to_cost.size()) {
      return true;
    } else {
      for (auto it : neighbor_id_to_cost) {
        router_id neighbor_id = it.first;
        cost_time neighbor_cost = it.second;
        if (currCostMapEntry.find(neighbor_id) == currCostMapEntry.end()) {
          return true;
        } else {
          if (currCostMapEntry[neighbor_id] != neighbor_cost) {
            return true;
          }
        }
      }
    }
    return false;
  }
}