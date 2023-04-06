#include "RoutingProtocolImpl.h"

int RoutingProtocolImpl::PingPongPacketSize = 12;

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n)
{
  sys = n;
  // add your own code
}

RoutingProtocolImpl::~RoutingProtocolImpl()
{
  // add your own code (if needed)
}

void RoutingProtocolImpl::sendPingPongPacket()
{
  int size = 8 + 4; // header + timestamp

  for (unsigned int i = 0; i < num_ports; i++)
  {
    char *PingPongPacket = (char *)malloc(sizeof(char) * size);
    *PingPongPacket = (unsigned char)PING;
    *(unsigned short *)(PingPongPacket + 2) = htons(size);                       // size
    *(unsigned short *)(PingPongPacket + 4) = htons(routerID);                   // source ID
    *(unsigned short *)(PingPongPacket + 6) = htons(portStatus[i].to_router_id); // destination ID
    *(unsigned int *)(PingPongPacket + 8) = htonl(sys->time());                  // timestamp
    sys->send(i, PingPongPacket, PingPongPacketSize);
  }
}

void RoutingProtocolImpl::handlePingPongPacket(port_number port, void *packet)
{
  char *data = reinterpret_cast<char *>(packet);
  ePacketType type = getPktType(packet);
  if (type == PING)
  {
    *data = (unsigned char)PONG;
    unsigned short target_id = *(unsigned short *)(data + 4);
    *(unsigned short *)(data + 4) = htons(routerID);
    *(unsigned short *)(data + 6) = htons(target_id);
    *(unsigned int *)(data + 8) = htonl(sys->time());
    sys->send(port, data, PingPongPacketSize);
  }
  else if (type == PONG)
  {
    // free(packet);
    time_stamp cur_time = sys->time();
    time_stamp timestamp = ntohl(*(unsigned int *)(data + 8));
    cost_time RTT = static_cast<cost_time>(cur_time - timestamp);
    router_id neighbor_id = ntohs(*(unsigned short *)(data + 4));

    // update ports table
    portStatus[port].to_router_id = neighbor_id;
    portStatus[port].cost = RTT;
    portStatus[port].last_update_time = cur_time;
    bool is_connect = portStatus[port].is_connected;
    portStatus[port].is_connected = true;

    // update direct neighbors table
    int RTT_diff = 0;
    bool has_port_info = false;
    if (neighbors.count(neighbor_id) && is_connect)
    { 
      // already connected before, just update cost
      neighbors[neighbor_id].port = port;
      cost_time old_RTT = neighbors[neighbor_id].cost;
      neighbors[neighbor_id].cost = RTT;
      RTT_diff = RTT - old_RTT;
      if (RTT_diff != 0)
      { // cost changes
        if (protocol_type == P_DV)
        {
          for (auto &entry : *(dv.DVTable))
          {
            if (entry.second.next_hop_id == neighbor_id)
            { 
              // is a next_hop of some destinations
              unsigned int new_RTT = entry.second.cost + RTT_diff;
              if (neighbors.count(entry.first) && neighbors[entry.first].cost < new_RTT)
              { 
                // now a direct neighbor is better
                entry.second.next_hop_id = entry.first;
                entry.second.cost = neighbors[entry.first].cost;
                forwardTable[entry.first] = entry.first;
              }
              else
              {                                // otherwise, use current route and just update cost
                entry.second.cost += RTT_diff; // may not best route anymore if cost increases
              }
              entry.second.last_update_time = sys->time();
            }
            else if (entry.first == neighbor_id && RTT < (*dv.DVTable)[neighbor_id].cost)
            { 
              // is a direct neighbor destination
              entry.second.next_hop_id = neighbor_id;
              entry.second.cost = RTT;
              forwardTable[neighbor_id] = neighbor_id;
              entry.second.last_update_time = sys->time();
            }
          }
          dv.sendPacket();
        }
      }
      else
      {
        if (protocol_type == P_DV)
        {
          for (auto entry : *dv.DVTable)
          {
            if (entry.second.next_hop_id == neighbor_id || entry.first == neighbor_id)
            {
              entry.second.last_update_time = sys->time();
            }
          }
        }
      }
    }
    else
    {
      has_port_info = true;
      neighbors[neighbor_id] = {port, static_cast<unsigned short>(RTT)};
      if (protocol_type == P_DV)
      {
        if (dv.DVTable->count(neighbor_id) && !is_connect)
        { 
          // re-connect a neighbor, may change best route for itself
          if (RTT < (*dv.DVTable)[neighbor_id].cost)
          { 
            // if direct route is better
            dv.updateDVTable(neighbor_id, RTT, neighbor_id);
            dv.sendPacket();
          }
          else
          {
            (*dv.DVTable)[neighbor_id].last_update_time = sys->time();
          }
        }
        else
        { // new neighbor that first time appears
          dv.insertDVEntry(neighbor_id, RTT, neighbor_id);
          dv.sendPacket();
        }
      }

      if (protocol_type == P_LS && (RTT_diff == 0 || !has_port_info))
      {
        ls.update_lsp_table();
        // cout << "after update" << endl;
        ls.sendPacket();
      }

      // find a new neighbor or re-connect
      forwardTable[neighbor_id] = neighbor_id;
    }
  }
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type)
{
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

  sendPingPongPacket();

  sys->set_alarm(this, 1000, expire_alarm);
  sys->set_alarm(this, 10 * 1000, ping_pong_alarm);

  if (protocol_type == P_DV)
  {
    sys->set_alarm(this, 30 * 1000, dv_alarm);
  }
  else
  {
    sys->set_alarm(this, 30 * 1000, ls_alarm);
  }

  dv.init(sys, router_id, num_ports, &neighbors, &portStatus, &forwardTable);
  ls.init(sys, router_id, num_ports, &neighbors, &portStatus, &forwardTable);
}

void RoutingProtocolImpl::handle_alarm(void *data)
{
  // add your own code
  eAlarmType type = (*((eAlarmType *)data));
  switch (type)
  {
  case SendPINGPONG:
    // generate ping pong message every 10 seconds
    sendPingPongPacket();
    sys->set_alarm(this, 10 * 1000, data);
    break;

  case SendDV:
    // send DV update every 30 seconds
    if (protocol_type == P_DV)
    {
      dv.sendPacket();
    }
    sys->set_alarm(this, 30 * 1000, data);
    break;

  case SendLS:
    // send LS update every 30 seconds
    if (protocol_type == P_LS)
    {
      ls.sendPacket();
    }
    sys->set_alarm(this, 30 * 1000, data);
    break;

  case SendCheck:
    // check link expiration every 1 second
    if (protocol_type == P_DV)
    {
      dv.checkLink();
    }
    else
    {
      if (port_check())
        ls.sendPacket();

      if (ls.remove_expired())
        ls.update_lsp_table();
    }
    sys->set_alarm(this, 1000, data);
    break;

  default:
    cerr << "unexpected alarm type" << endl;
    exit(1);
  }
}

bool RoutingProtocolImpl::port_check()
{
  bool is_expired = false;
  for (auto it = portStatus.begin(); it != portStatus.end(); ++it)
  {
    if (sys->time() - (it->second.last_update_time) > 15000)
    {
      it->second.cost = INFINITY_COST;
      it->second.is_connected = false;
      is_expired = true;
    }
  }
  return is_expired;
}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size)
{
  ePacketType t = getPktType(packet);

  switch (t)
  {
  case DATA:
    // cout << "receiving DATA..." << endl;
    sendData(port, packet);
    break;

  case PING: case PONG:
    // cout << "receiving PING/PONG..." << endl;
    handlePingPongPacket(port, packet);
    break;

  case DV:
    // cout << "receiving DV..." << endl;
    if (protocol_type == P_DV)
    {
      dv.recvPacket(port, packet, size);
    }
    else
    {
      cerr << "unexpected protocol msg" << endl;
      exit(1);
    }
    break;

  case LS:
    // cout << "receiving LS..." << endl;
    ls.recv_LSP(port, packet, size);
    break;

  default:
    cerr << "unexpected msg type " << t << endl;
    exit(1);
  }
}

void RoutingProtocolImpl::sendData(port_number port, void *packet)
{
  ePacketType t = getPktType(packet);
  if (t != DATA)
  {
    cerr << "packet should be" << t << endl;
    exit(1);
  }
  auto sp = reinterpret_cast<unsigned short *>(packet);
  router_id target_router_id = ntohs(*(sp + 3));
  if (target_router_id == routerID)
  {
    delete[] static_cast<char *>(packet);
    return;
  }

  if (forwardTable.find(target_router_id) == forwardTable.end())
  {
    return;
  }
  unsigned short size = *(reinterpret_cast<unsigned short *>(packet) + 1);
  router_id next_router = forwardTable[target_router_id];

  sys->send(neighbors[next_router].port, packet, size);
}
