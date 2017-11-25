#include <stdio.h>
#include "net.h"
#include "endecode.h"
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

int main() {
	int epollfd, listen_sock, conn_sock, nfds, i, flag = 0;
	struct epoll_event ev, events[MAX_CONNECTION];
	struct sockaddr_in cltaddr;

	int len = sizeof(cltaddr);

	epfd = epoll_create(MAX_CONNECTION);
	if(-1 ==  epollfd) {
		printf("error occurs in epoll_create()\n");
		return -1;
	}
	listen_sock = get_server_socket(PORT, MAX_CONNECTION);
	if(-1 == listen_sock) {
		return -1;
	}

     for( ; ; )
    {
        nfds = epoll_wait(epfd,events,MAX_CONNECTION, -1);
        for(i=0;i<nfds;++i)
        {
            if(events[i].data.fd==listen_sock //有新的连接
            {
                connfd = accept(listen_sock,(struct sockaddr*)&cltaddr, (socklen_t*)&len); //accept这个连接
                if(-1 == conn_sock){
					printf("error occurs in accept()\n");
					close(epfd);
					return -1;
				}
                ev.data.fd=connfd;
                ev.events=EPOLLIN|EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev); //将新的fd添加到epoll的监听队列中
            }
            else if( events[i].events&EPOLLIN ) //接收到数据，读socket
            {
                int ret, len, i;
                char *msg;

                ret = recv_msg(fd, &len, sizeof(int));
                if(0 == ret) {
                    printf("connection closed: sock %d\n", fd);
                    close(fd);
                    return;
                }

                msg = new char[len];
                ret = recv_msg(fd, msg, len);
                decode(msg,pkts);

                for(i=0; i<PORT_NUM; i++) {
                    print_packet(pkts[i]);
                }
                delete []msg;
                ev.data.ptr = md;     //md为自定义类型，添加数据
                ev.events=EPOLLOUT|EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);//修改标识符，等待下一个循环时发送数据，异步处理的精髓
            }
            else (events[i].events&EPOLLOUT) //有数据待发送，写socket
            {
                struct myepoll_data* md = (myepoll_data*)events[i].data.ptr;    //取数据
                sockfd = md->fd;
                send( sockfd, md->ptr, strlen((char*)md->ptr), 0 );        //发送数据
                ev.data.fd=sockfd;
                ev.events=EPOLLIN|EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev); //修改标识符，等待下一个循环时接收数据
            }

        }
    }


}
