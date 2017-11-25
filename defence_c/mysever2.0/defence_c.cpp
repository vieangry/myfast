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
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include "net.h"
#include "list.h"
#define OPEN_MAX 100
#define PORT 6666
#define MAX_CONNECTION 20
//feature.c
struct syn_flood{
  unsigned char data[54];
  unsigned char IPsrc[16];
  unsigned int IPsrcflag;
  unsigned char IPdst[16];
  unsigned int IPdst_synflag;
  unsigned char syn;
  unsigned char ack;
  unsigned char heibaiflag;
  long firsttime;
}
//抽取所有tcp流
//初始化结构体
//有目的ip和无目的ip的
//syn链接和ack链接
//初始化链表 match

//dIPdst_synflag某目的端syn连接数
Node* find_atacked_dst(Node *pNode){
	int max=0;
	Node *node = pNode;
	Node *nodemax = pNode;//最大数目的端节点
  if(sizeList(pHead)>100){
    while(node->next != NULL){
  		if(node->element.IPdst_synflag>max){
  			max=node->element.IPdst_synflag;
  			nodemax=node;
  		}
  	}
  	return nodemax;
  }
	return NULL;
}

unsigned char IPDST[16];//socket填
//填写头部//DOTO
int inittou(unsigned char *datatou){
  struct syn_flood *flood;
  flood->data[]=//
  flood->syn=//
  flood->ack=//
  flood->heibaiflag=0;//0不确定；１正常；-1异常
  flood->firsttime=getcurrenttieme();
  return 0;
}

Node *findIPsrc(Node *pNode,srtruct syn_flood *flood){
  if(pNode==NULL||flood==NULL){
    printf("erre:kong zhi zhen!\n");
    return NULL;
  }
  Node *head=pNode;
  while (head->next!=NULL) {
    if(ipv6_equal(head->IPsrc, flood->IPdst)==0){
      return head;
    }
    else{
      head=head->next;
    }
  }
  Node *pInsert;
  pInsert = (Node *)malloc(sizeof(Node));
  memset(pInsert,0,sizeof(Node));
  pInsert->element = flood;
  pInsert->next=NULL;
  head->next=insertnode;
  return insertnode;
}

//初始化节点
int initflood(Node *pNode,srtruct syn_flood *flood){
  int ret1=ipv6_equal(IPDST, flood->IPdst);
  Node *p=NULL;
  if(ret1==0&&(flood->syn==1||flood->ack==1)){
    //只有这两种报文我们才处理
    p=findIPsrc(pNode,flood);
    if(p!=NULL){
      if(p->heibaiflag==-1){
        IPdst_synflag++;
        //drop DOTO
        return 0;
      }else {
        p->IPsrcflag++;
        if(p->syn==1){
          IPdst_synflag++;
          cout_syn_connection(pNode,p);//处理syn太频繁
        }
        else if(p->ack==1){
          p->heibaiflag=1;
          //let go DOTO
          return 0;
        }
      }
    }
  }
  else
  //正常转发DOTO
  return 0;
}

void* thread()
{
  Node *head=pNode;//全局变量pNode
  while(head->next!=NULL){
    if(head->heibaiflag==-1||head->heibaiflag==1){
      head=head->next;
      continue;
    }else{
      if(getcurrenttieme()-head->firsttime>3){
        head->heibaiflag=-1;
      }
      head=head->next;
    }
  }
}

void dispose_func()
{
  pthread_t id;
  int temp,i;
  if((temp = pthread_create(&id, NULL, thread, NULL)) != 0){
    printf("thread creat failed!\n");
    return -1;
  }
  else
    printf("thread creat successful!\n");
  pthread_join(id,NULL);
  return 0;
}

void init_sigaction()
{
  struct sigaction act;
  act.sa_handler = dispose_func; //设置处理信号的函数
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  sigaction(SIGALRM, &act, NULL);//时间到发送SIGROF信号
}

void init_time()
{
  struct itimerval val;
  val.it_value.tv_sec = 3; //3秒后启用定时器
  val.it_value.tv_usec = 0;
  val.it_interval = val.it_value;
  setitimer(ITIMER_REAL, &val, NULL);
}
//某个syn链接超时处理，即：一条链接过了很长时间都没有收到第三次握手
int timeout(){
  //应该用多线程来做吧
  init_time();
  init_sigaction();
}

//IPDST 未定义
//unsigned char IPDST[16]="本端口ip";//初始化DOTO
int match(Node *pNode,srtruct syn_flood *flood){
  int ret1=ipv6_equal(IPDST, flood->IPdst);
  if(ret1==0&&(flood->syn==1||flood->ack==1)){
    //long nowtime =getcurrenttieme()；
    cout_syn_connection(pNode,flood);
    return 0;
  }
  return -1;
}

int cout_syn_connection(Node *pNode,struct syn_flood *flood1){
	if(flood1!=NULL&&flood1!=NULL){
		IPsrcflag++;
    if(flood1.hiebaiflag==0) {
			if(IPsrcflag==1){
        flood1.fisttime=getcurrenttieme();
      //let go
      return 0;
			}else if(IPsrcflag==2){
				flood1.secondtime=getcurrenttieme();
				if(flood1.secondtime-flood1.firsttime<3){
					flood1.heibaiflag=-1;
					//drop
          return 0;
				}else{
          //let go DOTO
          return 0;
        }
			}else if(IPsrcflag==3){
				flood1.3time=getcurrenttieme();
				if(flood1.3time-flood1.firsttime<6){
					flood1.heibaiflag=-1;
					//drop
          return 0;
				}else{
          //let go DOTO
          return 0;
        }
			}else if(IPsrcflag==4){
				flood1.4time=getcurrenttieme();
				if(flood1.4time-flood1.firsttime<12){
					flood1.heibaiflag=-1;
					//drop
          return 0;
				}else{
          //let go DOTO
          return 0;
        }
			}else if(IPsrcflag==5){
				flood1.5time=getcurrenttieme();
				if(flood1.5time-flood1.firsttime<24){
					flood1.heibaiflag=-1;
					//drop
				}else{
          //let go DOTO
          return 0;
        }
			}else {
        //drop
      }

		}else{
      //let go DOTO
    }
	}else//空指针
    return-1;
}
//feature.c
int main(int argc, char *argv[])
{
  struct epoll_event event;   // 告诉内核要监听什么事件
  struct epoll_event events[MAX_CONNECTION]; //内核监听完的结果
  int listen_sock = get_server_socket(PORT, MAX_CONNECTION);
	if(-1 == listen_sock) {
		return -1;
	}
  printf("listen_sock :%d\n",listen_sock);
  //4.epoll相应参数准备
  int epfd = epoll_create(MAX_CONNECTION);
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
    int nfds = epoll_wait(epfd, events, MAX_CONNECTION, -1);
    for(int i=0;i<nfds;i++){
        //6.1监测sockfd(监听套接字)是否存在连接
    if(( listen_sock== events[i].data.fd )
            && ( EPOLLIN == events[i].events & EPOLLIN ) )
    {
      struct sockaddr_in cli_addr;
      int clilen = sizeof(cli_addr);
      //6.1.1 从tcp完成连接中提取客户端
      int connfd = accept(listen_sock,(struct sockaddr*) &cli_addr,(socklen_t*) &clilen);
      if(-1 == connfd) {
        printf("error occurs in accept()\n");
        return -1;
        }
      printf("connfd:%d\n",connfd);
      //6.1.2 将提取到的connfd放入fd数组中，以便下面轮询客户端套接字
      event.data.fd = connfd; //监听套接字
      event.events = EPOLLIN; // 表示对应的文件描述符可以读
              //6.1.3.事件注册函数，将监听套接字描述符 connfd 加入监听事件
      ret = epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
      if(-1 == ret){
        perror("epoll_ctl");
          return -1;
      }
      printf("epoll_ctl:%d\n",ret);

    }
    else if(EPOLLIN == events[i].events & EPOLLIN )
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
        }
        else
          perror("read error:");
        printf("len<0\n");
      }
      else if(len == 0)//客户端关闭连接
      {
          close(fd[i]);
          printf("len=0\n");
      }
      else{//正常接收到服务器的数据
        //处理报文
      }
    }
  }
  return 0;
}
