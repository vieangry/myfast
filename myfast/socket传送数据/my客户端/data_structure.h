#include "../include/fast.h"

#define PORT_NUM 10
#define READ_INTERVAL 10000//读取计数器的间隔时间（微秒）

//每个READ_INTERVAL间隔内计数器统计的值：入字节数，出字节数，入报文数，出报文数
typedef struct counter_value{
	u64 c_in_byte;
	u64 c_out_byte;
	u64 c_in_pkt;
	u64 c_out_pkt;
}COUNTER_VALUE;

//储存特定端口所有最新的每秒字节数，每秒帧数，不对称性（字节），不对称性（报文）
typedef struct data_vector{
	double byte_ps;//每秒字节数
	double pkt_ps;//每秒报文数
	double asy_byte;//不对称性（字节）
	double asy_pkt;//不对称性（报文）
}DATA_VECTOR;

void print_data_vector(DATA_VECTOR v);

u64 PORT_REG(int port,u64 regaddr);

void read_all_counters(COUNTER_VALUE result[], int n);

COUNTER_VALUE read_counters(int port);

DATA_VECTOR caculate_data_vector(COUNTER_VALUE A);

DATA_VECTOR get_data_vector(int port);
