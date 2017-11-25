#include <stdio.h>
#include "net.h"

#define SERVER_IP "2001:da8:6000:306:f8e7:1707:5de0:2345"
#define CONNECTED_PORT 6666

int main(int argc, char *argv[])
{
	//fast_init_hw(0, 0);
	int sock = get_client_socket(CONNECTED_PORT, SERVER_IP);
	if(sock < 0) {
		printf("can't get right socket\n");
		return -1;
	}
	printf("connect to server successfully!\n");
	char buf[1024];
	while(1){
	    printf("jin ru while\n");
            
            char sendbuf[1024];
            scanf("输入：%s",&sendbuf);
            int va=send(sock,sendbuf,sizeof(sendbuf),0);
	    int val =recv_msg(sock, buf, 1024);printf("jin ru val\n");
		if(val > 0){
			buf[val] = '\0';
			printf("%s\n",buf);
			//deal_pkt(buf);解析报文，处理报文，
		}
		else
			continue;
	}

	close(sock);

	return 0;
}

