#include <stdio.h>
#include "net.h"
#include "data_structure.h"
#include "flow_message_packet.h"

#define SERVER_IP "2001:da8:6000:306:e27b:f596:4422:f2b3"
#define CONNECTED_PORT 6666

void analysis_mac(char mac[], char* str) {
	int flag = 0;
	int i = 0, j = 0;
	while(str[i] != '\0') {
		if((flag == 0 && str[i] == 'H') \
		|| (flag == 1 && str[i] == 'W') \
		|| (flag == 2 && str[i] == 'a') \
		|| (flag == 3 && str[i] == 'd') \
		|| (flag == 4 && str[i] == 'd') \
		|| (flag == 5 && str[i] == 'r') \
		|| (flag == 6 && str[i] == ' ')) {
			flag++;	
		} else if(flag == 7 && j < 12) {
			if(str[i] != ':') {
				mac[j] = str[i];
				j++;
			} else if(j == 12) break;
		}
		i++;
	}
}

void get_packets(FLOW_MESSAGE_PACKET packets[], int n) {
	int i;
	COUNTER_VALUE v[n];
	read_all_counters(v, n);
	
	for(i=0; i<n; i++) {
		packets[i].port = i;
		packets[i].in_flow = v[i].c_in_byte / READ_INTERVAL * 1000000;
		packets[i].out_flow = v[i].c_out_byte / READ_INTERVAL * 1000000;
	}
}

int main(int argc, char *argv[])
{
	int data_len;
	char data_send[PORT_NUM*FLOW_MESSAGE_LEN];
	
	FLOW_MESSAGE_PACKET packets[PORT_NUM];
	fast_init_hw(0, 0);
	int sock = get_client_socket(CONNECTED_PORT, SERVER_IP);
	if(sock < 0) {
		printf("can't get right socket\n");
		return -1;
	}
	
	printf("connect to server successfully!\n");
	char buf[1024];
	char mac[12] = {0};
	FILE *stream;

	stream = popen("ifconfig eth0", "r");
	fread(buf, sizeof(char), sizeof(buf), stream);
	pclose(stream);

	analysis_mac(mac, buf);
	
	send_msg(sock, mac, 12);

	int i;		
	while(1) {
		get_packets(packets, PORT_NUM);
		for(i=0; i<PORT_NUM; i++) {
			print_packet(packets[i]);
		}
		data_len = encode(packets, data_send);
		send_msg(sock, &data_len, sizeof(int));
		send_msg(sock, data_send, data_len);
	}

	close(sock);
	
	return 0;
}
