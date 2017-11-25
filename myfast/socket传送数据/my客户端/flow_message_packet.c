#include "flow_message_packet.h"
#include "data_structure.h"
#include <string.h>

int encode(FLOW_MESSAGE_PACKET msg[], char *pkt) {
	int len = PORT_NUM * FLOW_MESSAGE_LEN;
	char *ptr = pkt;
	int i;
	for(i=0; i<PORT_NUM; i++) {
		memcpy(ptr, &msg[i].port, sizeof(int));
		ptr += sizeof(int);
		memcpy(ptr, &msg[i].in_flow, sizeof(double));
		ptr += sizeof(double);
		memcpy(ptr, &msg[i].out_flow, sizeof(double));
		ptr += sizeof(double);
	}
	return len;
}

void decode(FLOW_MESSAGE_PACKET msg[], char *pkt) {
	char *ptr = pkt;
	int i;
	for(i=0; i<PORT_NUM; i++) {
		memcpy(&msg[i].port, ptr, sizeof(int));
		ptr += sizeof(int);
		memcpy(&msg[i].in_flow, ptr, sizeof(double));
		ptr += sizeof(double);
		memcpy(&msg[i].out_flow, ptr, sizeof(double));	
		ptr += sizeof(double);
	}
}

void print_packet(FLOW_MESSAGE_PACKET msg) {
	printf("port = %d\n", msg.port);
	printf("in_flow = %lf\n", msg.in_flow);
	printf("out_flow = %lf\n", msg.out_flow);
}
