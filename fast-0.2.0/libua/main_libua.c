/***************************************************************************
 *            main_libua.c
 *
 *  2017/01/05 13:20:17 星期四
 *  Copyright  2017  XuDongLai
 *  <XuDongLai0923@163.com>
 ****************************************************************************/
/*
 * main_libua.c
 *
 * Copyright (C) 2017 - XuDongLai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/fast.h"

#define UA_VERSION "1.0.2"

int recv_poll = 1;
int nlsk_recv = -1;
fast_ua_recv_callback ua_recv_callback = NULL;
struct msghdr netlink_msg;
struct nlmsghdr *nlh = NULL;
struct fast_ua_kernel_msg register_ua;
struct fast_ua_kernel_msg unregister_ua;
struct iovec iov;
struct sockaddr_nl src_addr, dest_addr;	
int uaf = 0;
void fast_ua_destroy(void);

/*
 *处理程序非正常退出时的最后工作，一定要在程序
 * 退出时进行清理工作(注销此进程的UA注册消息)
 * 否则，内核依然给此进程的PID发送消息
 */
void ua_improper_quit(int argc)
{
	void fast_ua_destroy();
	exit(0);
}

/*
 * FAST的UA进程向内核发起注册请求，并等待注册回应 
 * 消息，如果回应消息为注册成功，则设置信号处理并返回成功 
 * 否则返回失败
 */
static int fast_ua_register(int mid)
{

	int rc = 0,cur_pid = 0;
	
	cur_pid = getpid();
	//创建NetLink类型SOCKET
	nlsk_recv = socket(PF_NETLINK, SOCK_RAW, FAST_UA_NETLINK);
	if(nlsk_recv < 0)
	{
		FAST_ERR("Create FAST NetLink Socket Error!\n");
		return -E_UA_NLSK_CREATE;
	}
	//初始化数据为零
	memset(&netlink_msg, 0, sizeof(netlink_msg));
	memset(&src_addr, 0, sizeof(src_addr));
	//设置通信源端节点信息
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = cur_pid; 
	src_addr.nl_groups = 0;
	//SOCKET绑定本地节点
	rc = bind(nlsk_recv, (struct sockaddr*)&src_addr, sizeof(src_addr));
	if(rc < 0)
	{
		FAST_ERR("Bind FAST NetLink Socket Error!\n");
		return -E_UA_NLSK_BIND;
	}
	//设置通信目的节点信息，pid为0表示目的为：内核
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;
	dest_addr.nl_groups = 0;
	//初始化通信消息
	nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(FAST_UA_PKT_MAX_LEN));
	nlh->nlmsg_len = FAST_UA_REG_LEN;
	nlh->nlmsg_pid = cur_pid;
	nlh->nlmsg_flags = 0;
	//初始化通信注册数据(通信消息载体)
	register_ua.mid = mid;
	register_ua.pid = getpid();
	register_ua.state = UA_REG;
	//注销数据
	unregister_ua.mid = mid;
	unregister_ua.pid = register_ua.pid;
	unregister_ua.state = UA_UNREG;

	//设置通信参数
	memcpy(NLMSG_DATA(nlh),(char *)&register_ua,sizeof(struct fast_ua_kernel_msg));
	memset(&register_ua,0,sizeof(struct fast_ua_kernel_msg));
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	netlink_msg.msg_name = (void *)&dest_addr;
	netlink_msg.msg_namelen = sizeof(dest_addr);
	netlink_msg.msg_iov = &iov;
	netlink_msg.msg_iovlen = 1;

	//向内核发送NetLink消息，消息内容为注册数据结构
	sendmsg(nlsk_recv, &netlink_msg, 0);
	FAST_DBG("Register UA to FAST Kernel! Wait Reply......\n");
	
	//清空数据后准备接收
	memset(nlh, 0, NLMSG_SPACE(FAST_UA_PKT_MAX_LEN));
	//等待内核返回消息
	rc = recvmsg(nlsk_recv, &netlink_msg, 0);
	memcpy((char *)&register_ua,NLMSG_DATA(nlh),sizeof(register_ua));//FAST_UA_REG_LEN
	//判断返回消息是否注册成功
	if(register_ua.mid == mid && register_ua.pid == cur_pid && register_ua.state == UA_OK)//返回2说明注册成功
	{		
		FAST_DBG("UA->pid:%d,mid:%d,Register OK!\n",register_ua.pid,register_ua.mid);
		signal(SIGINT,ua_improper_quit);//非法结束时，调用注销函数
		return 0;
	}
	else
	{
		FAST_DBG("Register ERROR!return->pid:%d,mid:%d,state:%d\n",register_ua.pid,register_ua.mid,register_ua.state);
		return -E_UA_NLSK_REG_ERR;
	}	
}

/*
 * 提供给UA程序向内核发送消息，此消息可以是转发给其他UA进程，也可以是直接转发至硬件
 * 其中UM中的dstmid的取值为[1-10]时表示转发到其他UA进程，[11-254]表示转发到硬件，取值范围可再商定
*/
int fast_ua_send(struct fast_packet *pkt,int pkt_len)
{
	if(pkt->um.len == pkt_len && pkt_len >= 34 + 60 && pkt_len <= 34 + 1514){}
	else
	{
		printf("ERROR:um->len:%d,pkt_len:%d\n",pkt->um.len,pkt_len);
		exit(0);
	}
#if 1
	memcpy(NLMSG_DATA(nlh),pkt,pkt_len);
	nlh->nlmsg_len = pkt_len + 16;//除数据长度外再加上消息头长度
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	dest_addr.nl_family = AF_NETLINK;
	netlink_msg.msg_name = (void *)&dest_addr;
	netlink_msg.msg_namelen = sizeof(dest_addr);
	netlink_msg.msg_iov = &iov;
	netlink_msg.msg_iovlen = 1;

	/*检验发送长度是否与报文长度一致,不同表示发送失败*/
	if(sendmsg(nlsk_recv, &netlink_msg, 0) != nlh->nlmsg_len)/*要与对齐后的长度相比*/
	{
		printf("Send len ERROR!\n");
		return -1;
	}
	return pkt_len;
#else	
	return write(uaf,(char *)pkt,pkt_len);
#endif	
}

int fast_ua_file(void)
{
	if((uaf=open("/proc/openbox/ua",O_RDWR,0)) == -1)
	{
		printf("open UA file Err!\n");
		exit (-1);
	}
}

/*
 * FAST的UA程序初始化调用函数，需要UA程序提供两个参数
 * 参数一表示接收目的MID为此值的报文
 * 参数二表示接收到报文后通过此回调函数输出
 */
int fast_ua_init(int mid,fast_ua_recv_callback callback)
{
	int err = 0;
	/*向系统注册自己要接收报文的模块ID号*/
	if((err = fast_ua_register(mid)))
	{
		return err;
	}
	/*保存用户回调函数的指针信息*/
	ua_recv_callback = callback;
	//fast_ua_file();
	FAST_DBG("libua version:%s\n",UA_VERSION);
	return 0;
}

/*
 * UA程序正常或非法退出时执行的操作，向内核告知自己将要结束，让内核清除此MID号上的报文转发功能
 */
void fast_ua_destroy(void)
{
	recv_poll = 0;
	nlh->nlmsg_len = FAST_UA_REG_LEN;
	/*构建UA的注销报文*/
	memcpy(NLMSG_DATA(nlh),(char *)&unregister_ua,FAST_UA_REG_LEN);
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	dest_addr.nl_family = AF_NETLINK;
	netlink_msg.msg_name = (void *)&dest_addr;
	netlink_msg.msg_namelen = sizeof(dest_addr);
	netlink_msg.msg_iov = &iov;
	netlink_msg.msg_iovlen = 1;

	/*向系统发送注销信息*/
	sendmsg(nlsk_recv, &netlink_msg, 0);
	close(nlsk_recv);
	printf("fast_ua_destroy\n");
	exit(0);//结束程序运行
}

/*
 * 循环接收内核分派报文，并调用用户回调函数进行输出
 */
void recv_thread(void *argv)
{
	struct fast_packet *pkt = (struct fast_packet *)malloc(FAST_UA_PKT_MAX_LEN);
	int rc = 0;
	
	FAST_DBG("fast_ua_recv......\n");
	/*超时设置机制1
	int flag = fcntl(nlsk_recv,F_GETFL,0);
	fcntl(nlsk_recv,F_SETFL,flag|O_NONBLOCK);//非阻塞式，如果没有得到数据就返回，rc = -1
	*/
	/*超时设置机制2
	struct timeval timeout ={0,0};
	setsockopt(nlsk_recv,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
	*/
	
	while(recv_poll == 1)
	{
		memset(pkt, 0, FAST_UA_PKT_MAX_LEN);
		/*阻塞式接收内核分派报文*/
		rc = recv(nlsk_recv,pkt,FAST_UA_PKT_MAX_LEN, 0);
		if(rc >0 && ua_recv_callback != NULL)
		{	
			/*用户回调函数有效时,通过回调函数转给用户处理*/
			ua_recv_callback(pkt,rc);
		}
		else
		{
			FAST_DBG("time out!\n");
		}
	}
}
/*
 * UA程序启动数据接收函数，此函数执行后，后继代码继续往下执行
 */
void fast_ua_recv()//可以提供类似SOCKET接收函数，直接带数据返回的
{
	int ret = 0;
	pthread_t p_t;
	void *tret;
	/*创建接收报文线程*/
	ret = pthread_create(&p_t,NULL,(void *)recv_thread,NULL);
	if(ret != 0)
	{
		FAST_ERR("fast_ua_recv error!\n");
	}
	//pthread_join(p_t,&tret);
}
/*
 * UA程序启动数据接收函数，此函数执行后，代码暂停在函数结束处
 */
void fast_ua_recvhold()//可以提供类似SOCKET接收函数，直接带数据返回的
{
	int ret = 0;
	pthread_t p_t;
	void *tret;
	ret = pthread_create(&p_t,NULL,(void *)recv_thread,NULL);
	if(ret != 0)
	{
		FAST_ERR("fast_ua_recv error!\n");
	}
	pthread_join(p_t,&tret);
}

/*打印FAST结构报文数据*/
void print_pkt(struct fast_packet *pkt,int pkt_len)
{
	int i=0;
	printf("-----------------------***FAST PACKET***-----------------------\n");
	printf("Packet Addr:%p\n",pkt);
	for(i=0;i<16;i++)
	{
		if(i % 16 == 0)
			printf("      ");
		printf(" %X ",i);
		if(i % 16 == 15)
			printf("\n");
	}
	
	for(i=0;i<pkt_len;i++)
	{
		if(i % 16 == 0)
			printf("%04X: ",i);
		printf("%c ",*((u8 *)pkt+i));
		if(i % 16 == 15)
			printf("\n");
	}
	if(pkt_len % 16 !=0)
		printf("\n");
	printf("-----------------------***FAST PACKET***-----------------------\n\n");
}
