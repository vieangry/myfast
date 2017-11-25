#define FLOW_MESSAGE_LEN (sizeof(int)+2*sizeof(double))
#define PORT_NUM 10

//传递给应用的报文结构
typedef struct flow_message_packet {
	int port;
	double in_flow;
	double out_flow;
}FLOW_MESSAGE_PACKET;


//报文编解码函数

int encode(FLOW_MESSAGE_PACKET msg[], char *pkt);

void decode(FLOW_MESSAGE_PACKET msg[], char *pkt);

void print_packet(FLOW_MESSAGE_PACKET msg);
