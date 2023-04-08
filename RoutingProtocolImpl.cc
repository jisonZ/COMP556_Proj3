#include "RoutingProtocolImpl.h"

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
  sys = n;
  // add your own code
}

RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
}

void RoutingProtocolImpl::sendPingPacket() {
  for (port_number port_num = 0; port_num < num_ports; ++port_num) {
    char *pingPacket = new char[PING_PONG_PKT_SIZE];

    *pingPacket = (unsigned char)PING;
    *(unsigned short *)(pingPacket + 2) = (unsigned short)htons(PING_PONG_PKT_SIZE);
    *(unsigned short *)(pingPacket + 4) = (unsigned short)htons(routerID);  // send self router id
    *(unsigned int *)(pingPacket + 8) = (unsigned int)htonl(sys->time());

    // cout << "[sendPingPacket] routerID: " << routerID << "| timestamp: " << sys->time() << endl;

    sys->send(port_num, pingPacket, PING_PONG_PKT_SIZE);
  }
}

void RoutingProtocolImpl::sendPongPacket(port_number port, char *packet, unsigned short size) {
  char *pongPacket = new char[PING_PONG_PKT_SIZE];

  *pongPacket = (unsigned char)PONG;
  *(unsigned short *)(pongPacket + 2) = (unsigned short)htons(size);
  *(unsigned short *)(pongPacket + 4) = (unsigned short)htons(routerID);
  *(unsigned short *)(pongPacket + 6) = *(unsigned short *)(packet + 4);
  *(unsigned int *)(pongPacket + 8) = *(unsigned int *)(packet + 8);

  // cout << "[sendPongPacket] size" << size
  //      << " | routerID: " << routerID
  //      << " | destID: " << ntohs(*(unsigned short *)(packet + 4))
  //      << " | time: " << ntohs(*(unsigned int *)(packet + 8)) << endl;

  sys->send(port, pongPacket, PING_PONG_PKT_SIZE);
}

// void RoutingProtocolImpl::recvPongPacket(port_number port, char *packet)
// {
//   router_id destId = (unsigned short)ntohs(*(unsigned short *)(packet + 6));
//   if (destId != routerID)
//   {
//     free(packet);
//     return;
//   }

//   // free(packet);
//   time_stamp cur_time = sys->time();
//   time_stamp timestamp = (unsigned int)ntohl(*(unsigned int *)(packet + 8));
//   cost_time RTT = static_cast<cost_time>(cur_time - timestamp);
//   router_id neighbor_id = (unsigned short)ntohs(*(unsigned short *)(packet +
//   4));

//   // update ports table
//   portStatus[port].to_router_id = neighbor_id;
//   portStatus[port].cost = RTT;
//   portStatus[port].last_update_time = cur_time;
//   bool is_connect = portStatus[port].is_connected;
//   portStatus[port].is_connected = true;

//   // cout << "[receive pong] port: " << port << ", to_router_id: " <<
//   neighbor_id << ", cost: " << RTT << ", last_update_time: " << cur_time <<
//   endl;

//   // update direct neighbors table
//   int RTT_diff = 0;
//   bool has_port_info = false;
//   if (neighbors.count(neighbor_id) && is_connect)
//   {
//     // already connected before, just update cost
//     neighbors[neighbor_id].port = port;
//     cost_time old_RTT = neighbors[neighbor_id].cost;
//     neighbors[neighbor_id].cost = RTT;
//     RTT_diff = RTT - old_RTT;
//     if (RTT_diff != 0)
//     { // cost changes
//       if (protocol_type == P_DV)
//       {
//       }
//     }
//     else
//     {
//       if (protocol_type == P_DV)
//       {
//       }
//     }
//   }
//   else
//   {
//     has_port_info = true;
//     neighbors[neighbor_id] = {port, static_cast<unsigned short>(RTT)};
//     if (protocol_type == P_DV)
//     {

//     }

//     if (protocol_type == P_LS && (RTT_diff == 0 || !has_port_info))
//     {
//       ls.update_lsp_table();
//       ls.sendPacket();
//     }

//     // find a new neighbor or re-connect
//     forwardTable[neighbor_id] = neighbor_id;
//   }
// }

void RoutingProtocolImpl::recvPongPacket(port_number port, char *packet) {
  router_id destId = (unsigned short)ntohs(*(unsigned short *)(packet + 6));
  if (destId != routerID) {
    free(packet);
    return;
  }

  // free(packet);
  time_stamp currTime = sys->time();
  time_stamp timestamp = (unsigned int)ntohl(*(unsigned int *)(packet + 8));
  cost_time RTT = static_cast<cost_time>(currTime - timestamp);
  router_id neighborId = (unsigned short)ntohs(*(unsigned short *)(packet + 4));

  free(packet);

  if (protocol_type == P_DV) {
    // update portStatus table
    portStatus[port].to_router_id = neighborId;
    portStatus[port].cost = RTT;
    portStatus[port].last_update_time = currTime;
    bool isConnected = portStatus[port].is_connected;
    portStatus[port].is_connected = true;

    handleDVRecv(port, neighborId, RTT, isConnected);
  } else if (protocol_type == P_LS) {
    handleLSRecv(port, neighborId, RTT);
  } else {
    cout << "unexpected protocol type, abort." << endl;
    exit(1);
  }
}

void RoutingProtocolImpl::handleDVRecv(port_number port, router_id neighborId, cost_time RTT,
                                       bool isConnected) {
  // check if this neighbor is already connected before
  if (neighbors.find(neighborId) != neighbors.end() && isConnected) {
    // update connected neighbor's cost and port number
    neighbors[neighborId].port = port;
    cost_time oldRTT = neighbors[neighborId].cost;
    neighbors[neighborId].cost = RTT;
    cost_time rttDiff = RTT - oldRTT;

    // cost has changed
    if (rttDiff != 0) {
      for (auto it = dv.DVTable->begin(); it != dv.DVTable->end(); ++it) {
        it->second.last_update_time = sys->time();

        // router_id of current entry in DVTable
        router_id currRouterId = it->first;

        // current router use neighborId as next hop
        if (it->second.next_hop_id == neighborId) {
          cost_time newRTT = it->second.cost + rttDiff;
          // TODO: is this check necessary?
          // going to next_hop router is more expensive now -> go directly from
          // currRouterId instead
          if (neighbors.find(currRouterId) != neighbors.end() &&
              neighbors[currRouterId].cost < newRTT) {
            it->second.next_hop_id = currRouterId;
            it->second.cost = neighbors[currRouterId].cost;
            forwardTable[currRouterId] = currRouterId;
          } else {  // update cost
            it->second.cost = newRTT;
          }
        }
        // current router is a direct neighbor, and now it has a new min cost
        else if (currRouterId == neighborId && (*dv.DVTable)[neighborId].cost > RTT) {
          it->second.next_hop_id = neighborId;
          it->second.cost = RTT;
          forwardTable[neighborId] = neighborId;
        }
      }
      dv.sendPacket();
    }
    // cost has not changed, just update the timestamp for each entry
    else {
      for (auto it = dv.DVTable->begin(); it != dv.DVTable->end(); ++it) {
        if (it->second.next_hop_id == neighborId || it->first == neighborId) {
          it->second.last_update_time = sys->time();
        }
      }
    }
  }
  // neighbor is either new or re-connecting
  else {
    // re-connecting: DVTable has an entry for neighborId but port is not connected
    if (dv.DVTable->find(neighborId) != dv.DVTable->end() && !isConnected) {
      // TODO: do we need if-else here?
      if ((*dv.DVTable)[neighborId].cost > RTT) {
        dv.updateDVTable(neighborId, RTT, neighborId);
        dv.sendPacket();
      } else {
        (*dv.DVTable)[neighborId].last_update_time = sys->time();
      }
      // new neighbor
    } else {
      dv.insertDVEntry(neighborId, RTT, neighborId);
      dv.sendPacket();
    }
    forwardTable[neighborId] = neighborId;
  }
}

void RoutingProtocolImpl::handleLSRecv(port_number port, router_id neighborId, cost_time RTT) {
  bool portStatusUpdated = false;
  bool portStatusFound = portStatus.find(port) != portStatus.end();

  if (portStatusFound) {
    PortEntry oldPortStatus = portStatus[port];
    portStatusUpdated = oldPortStatus.cost != RTT || oldPortStatus.to_router_id != neighborId;
  }

  portStatus[port] = {neighborId, RTT, sys->time()};

  if (!portStatusFound || portStatusUpdated) {
    ls.update_lsp_table();
    ls.sendPacket();
  }
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id,
                               eProtocolType protocol_type) {
  // add your own code
  this->num_ports = num_ports;
  this->routerID = router_id;
  this->protocol_type = protocol_type;

  char *ping_pong_alarm = new char[sizeof(eAlarmType)];
  char *dv_alarm = new char[sizeof(eAlarmType)];
  char *ls_alarm = new char[sizeof(eAlarmType)];
  char *expire_alarm = new char[sizeof(eAlarmType)];

  *((eAlarmType *)ping_pong_alarm) = SendPINGPONG;
  *((eAlarmType *)dv_alarm) = SendDV;
  *((eAlarmType *)ls_alarm) = SendLS;
  *((eAlarmType *)expire_alarm) = SendCheck;

  sendPingPacket();

  sys->set_alarm(this, 1000, expire_alarm);
  sys->set_alarm(this, 10 * 1000, ping_pong_alarm);

  if (protocol_type == P_DV) {
    sys->set_alarm(this, 30 * 1000, dv_alarm);
  } else {
    sys->set_alarm(this, 30 * 1000, ls_alarm);
  }

  dv.init(sys, router_id, num_ports, &neighbors, &portStatus, &forwardTable);
  ls.init(sys, router_id, num_ports, &neighbors, &portStatus, &forwardTable);
}

void RoutingProtocolImpl::handle_alarm(void *data) {
  // add your own code
  eAlarmType type = (*((eAlarmType *)data));
  switch (type) {
    case SendPINGPONG:
      // generate ping pong message every 10 seconds
      sendPingPacket();
      sys->set_alarm(this, 10 * 1000, data);
      break;

    case SendDV:
      // send DV update every 30 seconds
      if (protocol_type == P_DV) {
        dv.sendPacket();
      }
      sys->set_alarm(this, 30 * 1000, data);
      break;

    case SendLS:
      // send LS update every 30 seconds
      if (protocol_type == P_LS) {
        ls.sendPacket();
      }
      sys->set_alarm(this, 30 * 1000, data);
      break;

    case SendCheck:
      // check link expiration every 1 second
      if (protocol_type == P_DV) {
        dv.checkLink();
      } else {
        if (port_check()) {
          ls.sendPacket();
        }
        if (ls.remove_expired()) {
          ls.update_lsp_table();
        }
      }
      sys->set_alarm(this, 1000, data);
      break;

    default:
      cerr << "unexpected alarm type" << endl;
      exit(1);
  }
}

bool RoutingProtocolImpl::port_check() {
  bool is_expired = false;
  for (auto it = portStatus.begin(); it != portStatus.end(); ++it) {
    if (sys->time() - (it->second.last_update_time) > 15000) {
      it->second.cost = INFINITY_COST;
      it->second.is_connected = false;
      is_expired = true;
    }
  }
  return is_expired;
}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
  ePacketType t = getPktType(packet);

  switch (t) {
    case DATA:
      sendData(port, packet);
      break;

    case PING:
      sendPongPacket(port, (char *)packet, size);
      break;

    case PONG:
      recvPongPacket(port, (char *)packet);
      break;

    case DV:
      if (protocol_type == P_DV) {
        dv.recvPacket(port, packet, size);
      } else {
        cerr << "unexpected protocol msg" << endl;
        exit(1);
      }
      break;

    case LS:
      ls.recv_LSP(port, packet, size);
      break;

    default:
      cerr << "unexpected msg type " << t << endl;
      exit(1);
  }
}

void RoutingProtocolImpl::sendData(port_number port, void *packet) {
  ePacketType t = getPktType(packet);
  cout << "Reciving Data" << endl;
  if (t != DATA) {
    cerr << "packet should be" << t << endl;
    exit(1);
  }
  // auto sp = reinterpret_cast<unsigned short *>(packet);
  // router_id target_router_id = ntohs(*(sp + 3));

  router_id target_router_id = (unsigned short)ntohs(*(unsigned short *)((char *)packet + 6));
  
  if (target_router_id == routerID) {
    // delete[] static_cast<char *>(packet);
    free(packet);
    return;
  }
  //cout << "OP" <<endl;
  if (forwardTable.find(target_router_id) == forwardTable.end()) {
    return;
  }
  // unsigned short size = *(reinterpret_cast<unsigned short *>(packet) + 1);

  unsigned short size = (unsigned short)ntohs(*(unsigned short *)((char *)packet + 2));
  
  router_id next_router = forwardTable[target_router_id];
  cout << next_router << endl;
  // for (auto &i : portStatus)
  // {
  //   cout << i.second.to_router_id << endl;
  // }
  port_number port_d = 0;
  for (port_d = 0; port_d < num_ports; port_d ++)
  {
    if (portStatus[port_d].to_router_id == next_router)
      break;
  }
  sys->send(port_d, packet, size);
  cout << "End of Reciving Data" << endl;
}
