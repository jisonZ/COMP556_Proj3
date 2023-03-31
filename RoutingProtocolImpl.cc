#include "RoutingProtocolImpl.h"

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
  sys = n;
  // add your own code
}

RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type) {
  // add your own code
    num_ports = num_ports;
    routerID = router_id;
    protocol_type = protocol_type;

    char *ping_pong_alarm = new char[sizeof(eAlarmType)];
    char *dv_alarm = new char[sizeof(eAlarmType)];
    char *expire_alarm = new char[sizeof(eAlarmType)];
    *((eAlarmType *)ping_pong_alarm) = SendPINGPONG;
    *((eAlarmType *)dv_alarm) = SendDV;
    *((eAlarmType *)expire_alarm) = SendExpire;

    createPingPongMessage();

    sys->set_alarm(this, 1000, expire_alarm);
    sys->set_alarm(this, 10 * 1000, ping_pong_alarm);
    sys->set_alarm(this, 30 * 1000, dv_alarm);
    
    dv.init(sys, router_id, num_ports, &neighbors, &portStatus, &forwardTable);
}

void RoutingProtocolImpl::handle_alarm(void *data) {
  // add your own code


}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
  // add your own code
  ePacketType t = getPktType(packet);
  switch (t) {
    case DATA:

    case PING:

    case PONG:


    case DV:
      if (protocol_type == P_DV) {
        dv.recvPacket(port, packet, size);
      } else {
        cerr << "unexpected protocol msg" << endl;
        exit(1);
      }

    case LS:
      //
    default:
      cerr << "unexpected msg type" << endl;
      exit(1);
  }
}

void RoutingProtocolImpl::sendData() {
  
}

