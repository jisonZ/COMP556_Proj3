CC = g++
COPTS = -std=c++11 -g -Wall 
LKOPTS =

OBJS =\
	Event.o\
	Link.o\
	Node.o\
	RoutingProtocolImpl.o\
	Simulator.o\
	DistanceVector.o\
	utils.o\
	LinkState.o

HEADRES =\
	global.h\
	Event.h\
	Link.h\
	Node.h\
	RoutingProtocol.h\
	Simulator.h\
	utils.h\
	DistanceVector.h\
	LinkState.h

%.o: %.cc
	$(CC) $(COPTS) -c $< -o $@

all:	Simulator

Simulator: $(OBJS)
	$(CC) $(LKOPTS) -o Simulator $(OBJS)

$(OBJS): global.h
Event.o: Event.h Link.h Node.h Simulator.h
Link.o: Event.h Link.h Node.h Simulator.h
Node.o: Event.h Link.h Node.h Simulator.h
Simulator.o: Event.h Link.h Node.h RoutingProtocol.h Simulator.h 
RoutingProtocolImpl.o: RoutingProtocolImpl.h DistanceVector.h LinkState.h
DistanceVector.o: DistanceVector.h
LinkState.o: LinkState.h
utils.o: utils.h

clean:
	rm -f *.o Simulator core.*

