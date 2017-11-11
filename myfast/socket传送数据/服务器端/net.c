#include "net.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


int send_msg(int sock, const void *buf, int len) {
	int ret = 0, tmp;
    while(len > ret) {
        tmp = send(sock, buf+ret, len-ret, 0);
        if(-1 == tmp) {
            printf("error occurs in Send()\n");
            return -1;
        }
        ret += tmp;
    }
    return ret;
}

int recv_msg(int sock, void *buf, int len) {
	int ret = 0, tmp;
   	while(len != ret) {
        tmp = recv(sock, buf+ret, len-ret, 0);
        if(0 > tmp && errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN) {
            printf("error occurs in recv_msg()\n");
            return -1;
        } else if(0 == tmp) {
	//connection closed
            return 0;
        } else if(0 > tmp && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)) {
		continue;
	}
        ret += tmp;
    }
    return ret;
}

int get_server_socket(int port, int maxsock) {
	int sock;
	struct sockaddr_in6 hostaddr;
	memset(&hostaddr, 0, sizeof(hostaddr));
	hostaddr.sin6_family = AF_INET6;  
	hostaddr.sin6_port = htons(port);  
	hostaddr.sin6_addr = in6addr_any;  
	
	sock = socket(AF_INET6, SOCK_STREAM, 0); 
	if(-1 == sock) {
		printf("error occurs in socket()\n");
		return -1;
	}
	
	if(bind(sock, (struct sockaddr*)&hostaddr, sizeof(hostaddr)) == -1) {
		printf("error occurs in bind()\n");
		close(sock);
		return -1;
	}
	
	if(listen(sock, maxsock) == -1) {
		printf("error occurs in listen()\n");
		close(sock);
		return -1;
	}
	return sock;
}

int get_client_socket(int port, char *ip) {
	int sock;
	struct sockaddr_in6 hostaddr;
	hostaddr.sin6_family = AF_INET6;  
	hostaddr.sin6_port = htons(port);  
	inet_pton(AF_INET6, ip, (void*)&hostaddr.sin6_addr);

	sock = socket(AF_INET6, SOCK_STREAM, 0);
	if(-1 == sock) {
		printf("error occurs in socket()\n");
		return -1;
	}
	
	if(connect(sock, (struct sockaddr *)&hostaddr, sizeof(hostaddr)) < 0) {
		printf("error occurs in connect()\n");
		close(sock);
		return -1;
	}
	
	return sock;
}
