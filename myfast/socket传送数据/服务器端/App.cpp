#include <stdio.h>
#include "net.h"
#include "Connection.h"
#include "flow_message_packet.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

#define MAX_CONNECTION 10
#define PORT 6666
#define SOCKTYPE_LS 0
#define SOCKTYPE_AS 1

using namespace std;

map<int, Connection> connections;

typedef struct sock_type {
	int type;
	int fd;
}SOCK_TYPE;

void add_in_array(int sock, struct epoll_event ev) {
	Connection c;
	c.set_sock(sock);
	c.set_event(ev);
	connections[sock] = c;
}

void free_array(int sock, int epollfd) {
	struct epoll_event ev = connections[sock].get_event();
	if(-1 == epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, &ev)) {
		printf("error occurs in free_array()\n");
	}
	SOCK_TYPE *t = (SOCK_TYPE*)ev.data.ptr;
	free(t);
}

void handle_application_socket(int epollfd, int fd) {
	int ret, len, i;
	char *msg;
	FLOW_MESSAGE_PACKET pkts[PORT_NUM];
	
	ret = recv_msg(fd, &len, sizeof(int));
	if(0 == ret) {
		printf("connection closed: sock %d\n", fd);
		free_array(fd, epollfd);
		close(fd);
		return;
	}

	msg = new char[len];
	ret = recv_msg(fd, msg, len);
	decode(pkts, msg);
	
	for(i=0; i<PORT_NUM; i++) {
		print_packet(pkts[i]);
	}
	delete []msg;
}

int main() {
	int epollfd, listen_sock, conn_sock, nfds, i, flag = 0;
	struct epoll_event ev, events[MAX_CONNECTION];
	struct sockaddr_in cltaddr;
	
	int len = sizeof(cltaddr);
	
	epollfd = epoll_create(MAX_CONNECTION);
	if(-1 ==  epollfd) {
		printf("error occurs in epoll_create()\n");
		return -1;
	}
	listen_sock = get_server_socket(PORT, MAX_CONNECTION);
	if(-1 == listen_sock) {
		return -1;
	}
	
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	SOCK_TYPE *t = (SOCK_TYPE*)malloc(sizeof(SOCK_TYPE));
	t->type = SOCKTYPE_LS;
	t->fd = listen_sock;
	ev.data.ptr = t;
	add_in_array(listen_sock, ev);
	
	if(-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev)) {
		close(epollfd);
		free_array(listen_sock, epollfd);
		printf("error occurs in epoll_ctl()\n");
		return -1;
	}
	
	for(;;) {
		nfds = epoll_wait(epollfd, events, MAX_CONNECTION, -1);
		if(-1 == nfds) {
			printf("error occurs in epoll_wait()\n");
			break;
		}

		for(i=0; i<nfds; i++) {
			SOCK_TYPE* ptr = (SOCK_TYPE*) events[i].data.ptr;
			if(SOCKTYPE_LS == ptr->type && events[i].events&EPOLLIN) {
				conn_sock = accept(listen_sock, (struct sockaddr*)&cltaddr, (socklen_t*)&len);
				if(-1 == conn_sock) {
					printf("error occurs in accept()\n");
					free_array(listen_sock, epollfd);
					close(epollfd);
					return -1;
				}
				
				char mac[12];
				recv_msg(conn_sock, mac, 12);
				
				ev.events = EPOLLIN;
				SOCK_TYPE* p = (SOCK_TYPE*)malloc(sizeof(SOCK_TYPE));
				p->type = SOCKTYPE_AS;
				p->fd = conn_sock;
				ev.data.ptr = p;
				add_in_array(conn_sock, ev);
				if(-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev)) {
					free_array(conn_sock, epollfd);
					close(p->fd);
					printf("error occurs in epoll_ctl()\n");
				}
				connections[conn_sock].set_mac(mac);
				printf("a new connection is built, from mac: %s\n", mac);
			} else if(SOCKTYPE_AS == ptr->type && events[i].events&EPOLLIN) {
				handle_application_socket(epollfd, ptr->fd);
			} else {
				printf("unknown situation occurs!\n");
				flag = -1;
				break;
			}
		}
		if(flag == -1) break;
	}
	
	free_array(listen_sock, epollfd);
	close(listen_sock);
	close(epollfd);
}
