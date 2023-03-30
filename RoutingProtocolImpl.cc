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
        dvmanager.recvPacket(port, packet, size);
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

