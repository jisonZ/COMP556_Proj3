using port_number = unsigned short;
using router_id = unsigned short;
using cost_time = unsigned short;
using time_stamp = unsigned int;

enum alarmType {
  SendPINGPONG,
  SendDV,
  SendLS,
  SendExpire,
};


struct Neighbor {
  port_number port;
  cost_time cost;
  Neighbor () {}
  Neighbor (port_number port, cost_time cost) : port(port), cost(cost) {}
};

struct PortEntry {
  router_id to_router_id;
  cost_time cost;
  time_stamp last_update_time;
  bool is_connected;
};

struct DVEntry{
  router_id next_hop_id;
  cost_time cost;
  time_stamp last_update_time;
  DVEntry() {}
  DVEntry(router_id next_hop_id, cost_time cost, time_stamp last_update_time): next_hop_id(next_hop_id), cost(cost), \ 
  last_update_time(last_update_time) {}
};