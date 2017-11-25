#include "../include/fast.h"
#include <stdio.h>
#include "net.h"

#define SERVER_IP "2001:da8:6000:306:f8e7:1707:5de0:2345"
#define CONNECTED_PORT 6666

struct defence_e_info{
	int i;
	unsigned char IPsrc[16];
}
//下流表
int down_flow(struct defenec_e_info e_info){
	struct fast_rule *rule={0};
	memcpy( rule->key.ipv6.src,e_info, sizeof(e_info));//不知道对不对
	memcpy( rule->mask.ipv6.src,1, sizeof(rule->mask.ipv6.src));
	fast_add_rule(rule);
	return 0;
}
int main(int argc, char *argv[])
{
	/*初始化平台硬件*/
	fast_init_hw(0,0);
	/*UA模块初始化	*/
	ua_init();
	//fast_init_hw(0, 0);
	int sock = get_client_socket(CONNECTED_PORT, SERVER_IP);
	if(sock < 0) {
		printf("can't get right socket\n");
		return -1;
	}
	printf("connect to server successfully!\n");
	char buf[17];
	while(1){
	  printf("jin ru while\n");
		struct defence_e_info e_info;
		int val = recv(sock, buf, sizeof(buf), 0);
		if(val > 0){
			if(buf[0]==1){
				memcpy( &e_info, buf+1, sizeof(buf) );
				down_flow(e_info);//下发流表函数
				//deal_pkt(buf);解析报文，处理报文，
			}
	  }
		else
			continue;
	}
	close(sock);
	return 0;
}
