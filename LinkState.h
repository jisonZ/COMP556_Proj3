#ifndef LINKSTATE_H
#define LINKSTATE_H

#include "Node.h"
#include "utils.h"
#include <queue>

using seqNum_pointer = unordered_map<router_id, int>*;
// struct cost_router_struct
// {
//     router_id routerId;
//     cost_time costTime;
//     cost_router_struct(router_id _routerId, cost_time _costTime) : routerId(_routerId), costTime(_costTime) {}
// };

// struct cmp 
// {
//     bool operator() (const cost_router_struct& r1, const cost_router_struct& r2)
//     {
//         return (r1.costTime < r2.costTime);
//     }
// };
struct link_state_path_info
{
    unsigned int cost;
    router_id prev_router_id;
    time_stamp last_update_time;
};


class LinkState {
public: 

    void init(Node* sys, router_id id, port_number num_port, neighbors_pointer neighbors, portStatus_pointer por, forwarding_pointer fwtp);

    void flood_LSP(port_number except_port_id, void* packet, unsigned short size);
    
    void recv_LSP(port_number port, void *packet, unsigned short size);

    void sendPacket();
    
    void update_lsp_table();

    router_id find_next_router(unordered_map<router_id, pair<cost_time, router_id>> distances, router_id dest_router);

    bool cost_map_updated(router_id neighbor_id, unordered_map<router_id, cost_time> neighbor_id_to_cost);

    bool remove_expired();

private:
    neighbors_pointer neighbors;
    portStatus_pointer portStatus;
    forwarding_pointer forwardingTable;
    Node *sys;
    router_id routerID;
    port_number numOfPorts;


    // Dijkstra algorithm Structure
    // router id -> [cost, previous router id, last update time]
    unordered_map<router_id, link_state_path_info> node_info;
    // router id -> [neighbor router id -> cost]
    unordered_map<router_id, unordered_map<router_id, cost_time>> cost_map;  // populated by Ping-Pong
    // router id -> sequence number
    unordered_map<router_id, unsigned int> seq_map; // contains sequence number for all the other routers in network

    // sequence number
    unsigned int SeqNum = 0;

};


#endif