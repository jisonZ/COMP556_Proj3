#include "Node.h"
#include "utils.h"

// struct neighborInfo {
//     unsigned short id;
//     unsigned int rrt;
// };

class DVManager {
public:


private:
    Node *sys;
    unordered_map<port_number, Neighbor>* neighbors; // pointer reference, initialized by RoutingProtodcolImpl.cc
    unordered_map<port_number, PortEntry>* portStatus;  // ...
    unordered_map<port_number, port_number>* fowardingTable; // ...
    unordered_map<port_number, DVEntry>* distanceVectorTable; // new
};