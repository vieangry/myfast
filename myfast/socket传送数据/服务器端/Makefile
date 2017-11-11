CC = g++
CFLAGS=-w -g
TARGET=app
OBJS=App.o Connection.o flow_message_packet.o net.o

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	
clean:
	rm -rf $(TARGET) $(OBJS)

App.o:App.cpp
	$(CC) $(CFLAGS) -c $^
Connection.o:Connection.cpp
	$(CC) $(CFLAGS) -c $^
flow_message_packet.o:flow_message_packet.c
	$(CC) $(CFLAGS) -c $^
net.o:net.c
	$(CC) $(CFLAGS) -c $^