#include "data_structure.h"

void print_data_vector(DATA_VECTOR v) {
	printf("byte_ps = %.2lf, pkt_ps = %.2lf, asy_byte = %.2lf, asy_pkt = %.2lf\n", v.byte_ps, v.pkt_ps, v.asy_byte, v.asy_pkt);
}

u64 PORT_REG(int port,u64 regaddr)
{	
	return fast_reg_rd(FAST_PORT_BASE|(FAST_PORT_OFT*port)|(regaddr<<3));
}

//读取所有端口计数
void read_all_counters(COUNTER_VALUE result[], int n) {
	COUNTER_VALUE array_v1[n], array_v2[n];
	int i;
	for(i=0; i<n; i++) {
		array_v1[i].c_in_byte = PORT_REG(i, FAST_COUNTS_RECV_OK);
		array_v1[i].c_out_byte = PORT_REG(i, FAST_COUNTS_SEND_OK);
	}
	usleep(READ_INTERVAL);
	for(i=0; i<n; i++) {
		array_v2[i].c_in_byte = PORT_REG(i, FAST_COUNTS_RECV_OK);
		array_v2[i].c_out_byte = PORT_REG(i, FAST_COUNTS_SEND_OK);
	}
	
	for(i=0; i<n; i++) {
		result[i].c_in_byte = array_v2[i].c_in_byte - array_v1[i].c_in_byte;
		result[i].c_out_byte = array_v2[i].c_out_byte - array_v1[i].c_out_byte;
	}
}

//读取端口计数器
COUNTER_VALUE read_counters(int port) {
	COUNTER_VALUE v1, v2, v;
	v1.c_in_pkt = PORT_REG(port, FAST_COUNTS_RCVSPKT) + PORT_REG(port, FAST_COUNTS_RCVMPKT) + PORT_REG(port, FAST_COUNTS_RCVBPKT);
	v1.c_out_pkt = PORT_REG(port, FAST_COUNTS_SNDSPKT) + PORT_REG(port, FAST_COUNTS_SNDMPKT) + PORT_REG(port, FAST_COUNTS_SNDBPKT);
	v1.c_in_byte = PORT_REG(port, FAST_COUNTS_RECV_OK);
	v1.c_out_byte = PORT_REG(port, FAST_COUNTS_SEND_OK);
	usleep(READ_INTERVAL);
	v2.c_in_pkt = PORT_REG(port, FAST_COUNTS_RCVSPKT) + PORT_REG(port, FAST_COUNTS_RCVMPKT) + PORT_REG(port, FAST_COUNTS_RCVBPKT);
	v2.c_out_pkt = PORT_REG(port, FAST_COUNTS_SNDSPKT) + PORT_REG(port, FAST_COUNTS_SNDMPKT) + PORT_REG(port, FAST_COUNTS_SNDBPKT);
	v2.c_in_byte = PORT_REG(port, FAST_COUNTS_RECV_OK);
	v2.c_out_byte = PORT_REG(port, FAST_COUNTS_SEND_OK);
	
	v.c_in_pkt = v2.c_in_pkt - v1.c_in_pkt;
	v.c_out_pkt = v2.c_out_pkt - v1.c_out_pkt;
	v.c_in_byte = v2.c_in_byte - v1.c_in_byte;
	v.c_out_byte = v2.c_out_byte - v1.c_out_byte;
	
	return v;
}

DATA_VECTOR caculate_data_vector(COUNTER_VALUE A) {
	DATA_VECTOR v;
	v.byte_ps = (A.c_in_byte + A.c_out_byte) / READ_INTERVAL * 1000000;
	v.pkt_ps = (A.c_in_pkt + A.c_out_pkt) / READ_INTERVAL * 1000000;
	if(A.c_out_byte == 0) {
		A.c_out_byte = 1;
	} 
	v.asy_byte = A.c_in_byte / A.c_out_byte;
	if(A.c_out_pkt == 0) {
		A.c_out_pkt = 1;
	} 
	v.asy_pkt = A.c_in_pkt / A.c_out_pkt;
	return v;
}

DATA_VECTOR get_data_vector(int port) {
	DATA_VECTOR v;
	v = caculate_data_vector(read_counters(port));
	return v;
}
