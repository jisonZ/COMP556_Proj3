#include "LinkState.h"
#include <cstring>
void LinkState::init(Node *sys, router_id id, port_number numPorts, portStatus_pointer ports, forwarding_pointer fwtp)
{
    this->sys = sys;
    this->routerID = id;
    this->numPorts = numPorts;
    // this->neighbors = neighbors;
    this->portStatus = ports;
    this->forwardingTable = fwtp;
}

void LinkState::recvLSP(port_number port, void *packet, pkt_size size)
{
    unordered_map<router_id, cost_time> costMapEntry;
    seq_num incomingSeq;
    router_id sourceID;
    
    bool canweupdate = getLSinfo(packet, costMapEntry, incomingSeq, sourceID);
    if (!canweupdate) return;
    // if (sourceID != routerID)
    // {
    //     free(packet);
    //     return;
    // }
    //cout << "Ciallo2" << endl;
    if (seq_num_map.find(sourceID) == seq_num_map.end() || seq_num_map[sourceID] < incomingSeq)
    {
        seq_num_map[sourceID] = incomingSeq;
        
        if (hasCostMapChanged(sourceID, costMapEntry))
        {
            cost_map[sourceID] = costMapEntry;
            updateLSTable();
        }
        
        floodLSP(port, packet, size);
    }
    
}

void LinkState::floodLSP(port_number except_port_id, void *packet, pkt_size size)
{
    for (port_number portID = 0; portID < numPorts; ++portID)
    {
        // avoid flooding LSP to incoming port
        if (portID == except_port_id)
            continue;
        char *new_data = strdup(static_cast<char *>(packet));
        sys->send(portID, new_data, size);
    }
    free(packet);
}

void LinkState::sendLSP()
{
    unsigned int size = 24;
    // unsigned int size = (*neighbors).size() * 4 + 12;

    for (port_number port_id = 0; port_id < numPorts; ++port_id)
    {
        // continue if port is detached
        auto pit = portStatus->find(port_id);
        if (pit == portStatus->end() || !(pit->second.is_connected))
            continue;

        char *msg = new char[size];
        *msg = (unsigned char)LS;
        auto packet = (unsigned short *)msg;
        *(packet + 1) = htons(size);
        *(packet + 2) = htons(routerID);
        *((unsigned int *)(packet + 4)) = htonl(sequenceNum);

        uint32_t index = 0;

        // iterate neighbor
        // for (auto &nei : (*neighbors))
        // {
        //     auto neighbor_id = nei.first;
        //     auto cost = nei.second.cost;

        //     *(packet + 6 + index++) = htons(neighbor_id);
        //     *(packet + 6 + index++) = htons(cost);
        // }
        sys->send(port_id, msg, size);
        
    }
    ++sequenceNum;
}

void LinkState::updateLSTable()
{
    // <node, <cost, predecessor>>
    unordered_map<router_id, pair<cost_time, router_id>> distances;

    unordered_set<router_id> unvisited_routers;
    // for (auto it = cost_map[1].begin(); it != cost_map[1].end(); ++it)
    // {
    //     cout << it->first<< " " << it->second << endl;
    // }
    // cout << "End Cost Map" << endl;

    // Add adjacent routers to root router to D
    for (auto it = portStatus->begin(); it != portStatus->end(); ++it)
    {
        const PortEntry &entry = it->second;
        if (entry.is_connected)
        {
            distances[entry.to_router_id] = make_pair(entry.cost, routerID);
        }
    }

    // Add not directly connected routers to D
    for (auto it = cost_map.begin(); it != cost_map.end(); ++it)
    {
        router_id id = it->first;
        if (id == routerID)
            continue;
        unvisited_routers.insert(id);
        if (distances.find(id) == distances.end()) // Not connected to root router
            distances[id] = make_pair(INFINITY_COST, routerID);
    }

    // for (auto it = distances.begin(); it != distances.end(); ++it)
    // {
    //     cout << it->first << " < " << it->second.first << ", " << it->second.second << " > " << endl;
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
    while (!unvisited_routers.empty())
    {
        // cout << "---Begin---" << endl;
        // Find the shortest router to root router
        min_cost = INFINITY_COST;
        for (auto it = unvisited_routers.begin(); it != unvisited_routers.end(); ++it)
        {
            router_id unv_router = *it;
            cost_time router_cost = distances[unv_router].first;
            if (min_cost > router_cost)
            {
                cur_router_id = unv_router;
                min_cost = router_cost;
            }
        }

        // There is no conneceted components to root
        if (min_cost == INFINITY_COST)
        {
            break;
        }

        // Remove
        unvisited_routers.erase(cur_router_id);

        // Update Connection if it is shorter than cache
        auto cur_router_info = cost_map[cur_router_id];
        for (auto it = cur_router_info.begin(); it != cur_router_info.end(); ++it)
        {
            router_id nbr_id = it->first;
            cost_time nbr_cost = it->second;
            if (unvisited_routers.find(nbr_id) != unvisited_routers.end())
                if (distances[nbr_id].first > nbr_cost + min_cost)
                    distances[nbr_id] = make_pair(nbr_cost + min_cost, cur_router_id);
        }
    }

    // Update Node Info and Forwarding Table
    for (auto it = distances.begin(); it != distances.end(); ++it)
    {

        router_id cur_id = it->first;
        pair<cost_time, router_id> d = it->second;
        if (d.first == INFINITY_COST)
            continue;
        LSEntry cur_router_info;
        cur_router_info.cost = d.first;
        cur_router_info.prev_router_id = d.second;
        cur_router_info.last_update_time = sys->time();
        LSTable[cur_id] = cur_router_info;
        (*forwardingTable)[cur_id] = getNextHop(distances, cur_id);
    }
    //cout << "LS" << endl;
}

router_id LinkState::getNextHop(unordered_map<router_id, pair<cost_time, router_id>> distances, router_id dest_router)
{
    router_id cur_router_id = dest_router;
    while (distances[cur_router_id].second != routerID)
        cur_router_id = distances[cur_router_id].second;
    return cur_router_id;
}

// check and remove LS entries in LSTable which are older than 45 seconds
bool LinkState::isExpiredLSEntryRemoved()
{
    unsigned int now;
    vector<router_id> expired; // store router id of expired LS entries

    for (auto it : this->LSTable)
    {
        now = sys->time();
        if (now - it.second.last_update_time >= 45 * 1000)
        {
            expired.push_back(it.first);
        }
    }

    for (router_id routerId : expired)
    {
        this->LSTable.erase(routerId);
    }

    return !expired.empty();
}

// check if [neighbor id, cost] info in received LSP packet is different from cost_map
bool LinkState::hasCostMapChanged(router_id neighbor_id, unordered_map<router_id, cost_time> neighbor_id_to_cost)
{
    // cost_map does not contain info from this neighbor yet
    if (this->cost_map.find(neighbor_id) == this->cost_map.end())
    {
        return !neighbor_id_to_cost.empty();
    }
    else
    {
        // cost_map entry
        // cost_map: [router_id -> [neighbor_router_id -> cost]]
        cost_map_entry currCostMapEntry = this->cost_map[neighbor_id];
        if (currCostMapEntry.size() != neighbor_id_to_cost.size())
        {
            return true;
        }
        else
        {
            for (auto it : neighbor_id_to_cost)
            {
                router_id neighbor_id = it.first;
                cost_time neighbor_cost = it.second;
                if (currCostMapEntry.find(neighbor_id) == currCostMapEntry.end())
                {
                    return true;
                }
                else
                {
                    if (currCostMapEntry[neighbor_id] != neighbor_cost)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }
}