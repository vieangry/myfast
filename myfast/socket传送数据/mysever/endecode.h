#define FLOW_MESSAGE_LEN (sizeof(int)+2*sizeof(double))
#define PORT_NUM 10


//报文编解码函数

int encode(int a[10], char *pkt);

void decode(char *pkt,int b[10]);
