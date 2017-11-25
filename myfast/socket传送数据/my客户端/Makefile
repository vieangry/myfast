CC = gcc
CFLAGS=-w -g
TARGET=data_collector
LIB = -L ../include -L ../libreg\
	-L../librule \
	-L../libua \
	-lrule \
	-lua \
	-lreg 
	
OBJS=data_collector.o data_structure.o flow_message_packet.o net.o

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)
	
clean:
	rm -rf $(TARGET) $(OBJS)

data_collector.o:data_collector.c
	$(CC) $(CFLAGS) -c $^
data_structure.o:data_structure.c
	$(CC) $(CFLAGS) -c $^
flow_message_packet.o:flow_message_packet.c
	$(CC) $(CFLAGS) -c $^
net.o:net.c
	$(CC) $(CFLAGS) -c $^
