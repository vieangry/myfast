#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <errno.h>
#include "net.h"
#define OPEN_MAX 100
#define PORT 6666
#define MAX_CONNECTION 20

int main(int argc, char *argv[])
{
  struct epoll_event event;   // 告诉内核要监听什么事件
  struct epoll_event wait_event; //内核监听完的结果

  int listen_sock = get_server_socket(PORT, MAX_CONNECTION);
	if(-1 == listen_sock) {
		return -1;
	}
  printf("listen_sock :%d\n",listen_sock);
  //4.epoll相应参数准备
  int fd[OPEN_MAX];
  int i = 0, maxi = 0;
  memset(fd,-1, sizeof(fd));
  fd[0] = listen_sock;

  int epfd = epoll_create(10); // 创建一个 epoll 的句柄，参数要大于 0， 没有太大意义
  if( -1 == epfd ){
      perror ("epoll_create");
      return -1;
  }
  printf("epfd:%d\n",epfd);
  event.data.fd = listen_sock;     //监听套接字
  event.events = EPOLLIN; // 表示对应的文件描述符可以读

  //5.事件注册函数，将监听套接字描述符 sockfd 加入监听事件
  int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &event);
  if(-1 == ret){
      perror("epoll_ctl");
      return -1;
  }
  printf("epoll_ctl:%d\n",ret);
  //6.对已连接的客户端的数据处理
  while(1)
  {
    printf("jin ru while\n");
      // 监视并等待多个文件（标准输入，udp套接字）描述符的属性变化（是否可读）
      // 没有属性变化，这个函数会阻塞，直到有变化才往下执行，这里没有设置超时
      ret = epoll_wait(epfd, &wait_event, maxi+1, -1);

      //6.1监测sockfd(监听套接字)是否存在连接
      if(( listen_sock== wait_event.data.fd )
          && ( EPOLLIN == wait_event.events & EPOLLIN ) )
      {
        struct sockaddr_in6 cli_addr;
        int clilen = sizeof(cli_addr);

        //6.1.1 从tcp完成连接中提取客户端
        int connfd = accept(listen_sock,(struct sockaddr*) &cli_addr,(socklen_t*) &clilen);
        printf("connfd:%d\n",connfd);
        //6.1.2 将提取到的connfd放入fd数组中，以便下面轮询客户端套接字
        for(i=1; i<OPEN_MAX; i++)
        {
            if(fd[i] < 0)
            {
                fd[i] = connfd;
                event.data.fd = connfd; //监听套接字
                event.events = EPOLLIN; // 表示对应的文件描述符可以读

                //6.1.3.事件注册函数，将监听套接字描述符 connfd 加入监听事件
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
                if(-1 == ret){
                    perror("epoll_ctl");
                    return -1;
                }
               printf("epoll_ctl:%d\n",ret);
                break;
            }
        }

        //6.1.4 maxi更新
        if(i > maxi)
            maxi = i;
          printf("maxi:%d\n",maxi);
        //6.1.5 如果没有就绪的描述符，就继续epoll监测，否则继续向下看
        if(--ret <= 0)
            continue;
    }

    //6.2继续响应就绪的描述符
    for(i=1; i<=maxi; i++)
    {
        if(fd[i] < 0)
            continue;

        if(( fd[i] == wait_event.data.fd )
        && ( EPOLLIN == wait_event.events & (EPOLLIN|EPOLLERR) ))
        {
          int len = 0;
          char buf[128] = "";
          printf("接收客户端数据\n");
          //6.2.1接受客户端数据
          if((len = recv(fd[i], buf, sizeof(buf), 0)) < 0)
          {
              if(errno == ECONNRESET)//tcp连接超时、RST
              {
                  close(fd[i]);
                  fd[i] = -1;
              }
              else
                  perror("read error:");
              printf("len<0\n");
          }
          else if(len == 0)//客户端关闭连接
          {
              close(fd[i]);
              fd[i] = -1;
              printf("len=0\n");
          }
          else{//正常接收到服务器的数据
            printf("len>0:%s\n",buf);
              //send(fd[i], buf, len, 0);
          }
          char sendbuf[1024];
          scanf("输入:%s",&sendbuf);
          send(fd[i],sendbuf,sizeof(sendbuf),0);

          //6.2.2所有的就绪描述符处理完了，就退出当前的for循环，继续poll监测
          if(--ret <= 0)
              break;
      }
    }
  }
  return 0;
}
