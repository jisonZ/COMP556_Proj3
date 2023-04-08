#include "RoutingProtocolImpl.h"

#include <cstring>

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
  sys = n;
  // add your own code
}

RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id,
                               eProtocolType protocol_type) {
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
  } else if (protocol_type == P_LS) {
    sys->set_alarm(this, 30 * 1000, ls_alarm);
  } else {
    cerr << "unexpected protocol type. aborting." << endl;
    exit(1);
  }

  dv.init(sys, router_id, num_ports, &neighbors, &portStatus, &forwardingTable);
  ls.init(sys, router_id, num_ports, &portStatus, &forwardingTable);
}

void RoutingProtocolImpl::handle_alarm(void *data) {
  eAlarmType alarmType = (*((eAlarmType *)data));
  switch (alarmType) {
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
        ls.sendLSP();
      }
      sys->set_alarm(this, 30 * 1000, data);
      break;

    case SendCheck:
      // check link expiration every 1 second
      if (protocol_type == P_DV) {
        dv.checkLink();
      } else {
        if (ls.checkLink()) {
          ls.sendLSP();
        }
        if (ls.isExpiredLSEntryRemoved()) {
          ls.updateLSTable();
        }
      }
      sys->set_alarm(this, 1000, data);
      break;

    default:
      cerr << "unexpected alarm type: " << alarmType << endl;
      exit(1);
  }
}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
  switch (getPacketType(packet)) {
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
        cerr << "unexpected protocol type. aborting." << endl;
        exit(1);
      }
      break;

    case LS:
      if (protocol_type == P_LS) {
        ls.recvLSP(port, packet);
      } else {
        cerr << "unexpected protocol type. aborting." << endl;
        exit(1);
      }
      break;

    default:
      cerr << "unexpected msg type: " << getPacketType(packet) << ". aborting." << endl;
      exit(1);
  }
}

void RoutingProtocolImpl::sendPingPacket() {
  for (port_number port_num = 0; port_num < num_ports; ++port_num) {
    char *pingPacket = new char[PING_PONG_PKT_SIZE];

    *pingPacket = (unsigned char)PING;
    *(unsigned short *)(pingPacket + 2) = (unsigned short)htons(PING_PONG_PKT_SIZE);
    *(unsigned short *)(pingPacket + 4) = (unsigned short)htons(routerID);  // send self router id
    *(unsigned int *)(pingPacket + 8) = (unsigned int)htonl(sys->time());

    sys->send(port_num, pingPacket, PING_PONG_PKT_SIZE);
  }
}

void RoutingProtocolImpl::sendPongPacket(port_number port, char *packet, pkt_size size) {
  char *pongPacket = new char[PING_PONG_PKT_SIZE];

  *pongPacket = (unsigned char)PONG;
  *(unsigned short *)(pongPacket + 2) = (unsigned short)htons(size);
  *(unsigned short *)(pongPacket + 4) = (unsigned short)htons(routerID);
  // put source router id in PING packet as destination router id in PONG packet
  *(unsigned short *)(pongPacket + 6) = *(unsigned short *)(packet + 4);
  // put timestamp in PING packet
  *(unsigned int *)(pongPacket + 8) = *(unsigned int *)(packet + 8);

  sys->send(port, pongPacket, PING_PONG_PKT_SIZE);
}

void RoutingProtocolImpl::recvPongPacket(port_number port, char *packet) {
  router_id destId = (unsigned short)ntohs(*(unsigned short *)(packet + 6));
  if (destId != routerID) {
    free(packet);
    return;
  }

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

    dv.recvPong(port, neighborId, RTT, isConnected);

  } else if (protocol_type == P_LS) {
    ls.recvPong(port, neighborId, RTT);

  } else {
    cerr << "unexpected protocol type. aborting." << endl;
    exit(1);
  }
}

void RoutingProtocolImpl::sendData(port_number port, void *packet) {
  if (getPacketType(packet) != DATA) {
    cerr << "packet type should be DATA, but got: " << getPacketType(packet) << endl;
    free(packet);
    exit(1);
  }

  router_id target_router_id = (unsigned short)ntohs(*(unsigned short *)((char *)packet + 6));
  pkt_size size = (unsigned short)ntohs(*(unsigned short *)((char *)packet + 2));

  switch (protocol_type) {
    case P_DV:
      dv.sendData(target_router_id, size, port, packet);
      break;

    case P_LS:
      ls.sendData(target_router_id, size, port, packet);
      break;

    default:
      cerr << "unexpected protocol type. aborting." << endl;
      free(packet);
      exit(1);
  }
}