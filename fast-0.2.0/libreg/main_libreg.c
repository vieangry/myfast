/***************************************************************************
 *            main_libreg.c
 *
 *  2017/02/15 16:36:51 星期三
 *  Copyright  2017  XuDongLai
 *  <XuDongLai0923@163.com>
 ****************************************************************************/
/*
 * main_libreg.c
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

#define REG_VERSION "1.0.0"

#ifdef NetMagic08
#include <linux/ip.h>
#include <arpa/inet.h>
int exit_flag = 0;
int nm_skt = 0,recv_skt = 0;
struct sockaddr_in nm_addr;
struct recv_pkt
{
	struct ethhdr eth;
	struct iphdr ip;
	struct nm_packet nh;	
}__attribute__((packed));


void print_nmac_pkt(struct recv_pkt *pkt,int pkt_len)
{
	int i=0;
	printf("-----------------------***NMAC PACKET***-----------------------\n");
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
		printf("%02X ",*((u8 *)pkt+i));
		if(i % 16 == 15)
			printf("\n");
	}
	if(pkt_len % 16 !=0)
		printf("\n");
	printf("-----------------------***NMAC PACKET***-----------------------\n\n");
}

int recv_nm_pkt(struct recv_pkt *pkt,int *len)
{	
	while(!exit_flag && (*len = recv(recv_skt,(u8 *)pkt, sizeof(struct recv_pkt),0)) > 0)
	{
		if(pkt->ip.protocol == NMAC_PROTO)
		{
			return 1;
		}
		else
		{
			//print_nmac_pkt(pkt,*len);
			//printf("pkt len:%d,proto:%d\n",*len,pkt->ip.protocol);
		}
	}		
	return 0;
}

int send_nm_pkt(struct nm_packet *pkt,int len)
{
	return (len == sendto(nm_skt,(u8 *)pkt, len,0,(struct sockaddr*)&nm_addr,sizeof(nm_addr)));	
}


void *send_thread(void *argv)
{
	struct nm_packet *pkt = (struct nm_packet *)argv;	
	if(!send_nm_pkt(pkt,sizeof(struct nm_packet)))
	{
		FAST_ERR("Send NMAC Failed!\n");		
	}
	free(pkt);
}


int nm_connect(void)
{
	struct nm_packet *pkt = (struct nm_packet *)malloc(sizeof(struct nm_packet));
	struct recv_pkt *pkt2 = (struct recv_pkt *)malloc(sizeof(struct recv_pkt));
	int len = 0,ret = -1;
	pthread_t tid;
	
	pkt->nm.count = 1;
	pkt->nm.type = NM_CONN;
	if(pthread_create(&tid, NULL,send_thread, (void *)pkt))
	{
		FAST_ERR("Send NMAC CN PKT Failed!\n");
	}
	if(recv_nm_pkt(pkt2,&len) && pkt2->nh.nm.type == NM_CONN)
	{
		ret = 0;
	}
	free(pkt2);
	FAST_DBG("REG Version:%s,NetMagic08 HW Version:%lX\n",REG_VERSION,fast_reg_rd(FAST_HW_REG_VERSION));
	return ret;
}

void nm_release(void)
{
	struct nm_packet *pkt = (struct nm_packet *)malloc(sizeof(struct nm_packet));
	struct recv_pkt *pkt2 = (struct recv_pkt *)malloc(sizeof(struct recv_pkt));
	int len = 0,ret = -1;
	pthread_t tid;
	
	pkt->nm.count = 1;
	pkt->nm.type = NM_RELESE;
	if(pthread_create(&tid, NULL,send_thread, (void *)pkt))
	{
		FAST_ERR("Send NMAC RELESE PKT Failed!\n");
	}
	if(recv_nm_pkt(pkt2,&len) && pkt2->nh.nm.type == NM_RELESE)
	{
		FAST_DBG("NMAC RELEASE OK!\n");
	}
	free(pkt2);	
}

void __distroy_NetMagic08(int argc)
{
	exit_flag = 1;
	nm_release();
}

int init_hw_NetMagic08(u64 addr,u64 len)
{
	struct sockaddr_in sa;
	struct timeval recv_timeout={2,0};/*数据接收超时设置*/
	int ret = 0;
	
	bzero(&sa, sizeof(sa)); 
	bzero(&nm_addr, sizeof(nm_addr)); 
	sa.sin_family = AF_INET; 
	sa.sin_addr.s_addr = htonl(INADDR_ANY); 
	sa.sin_port = htons(123);

	nm_addr.sin_family = AF_INET; 
	nm_addr.sin_addr.s_addr = inet_addr("136.136.136.136");
	nm_addr.sin_port = htons(321); 
	/* 创建socket */
	if ((nm_skt = socket(AF_INET, SOCK_RAW,NMAC_PROTO)) < 0)//AF_INET,NMAC_PROTO,IPPROTO_ICMP
	{
		FAST_ERR("NMAC Send SKT Failed!\n");		
	}

	if ((recv_skt = socket(PF_PACKET, SOCK_RAW,htons(ETH_P_IP))) < 0)//AF_INET,NMAC_PROTO,IPPROTO_ICMP
	{
		FAST_ERR("NMAC Recv SKT Failed!\n");
	}	
	ret = setsockopt(recv_skt,SOL_SOCKET,SO_RCVTIMEO,(const char*)&recv_timeout,sizeof(recv_timeout));

	/* 绑定套接口 */
	if((bind(nm_skt,(struct sockaddr*)&sa,sizeof(sa))) == -1) 
	{ 
		FAST_ERR("NMAC Bind Failed!"); 
	}
	
	signal(SIGINT,__distroy_NetMagic08);				/*非法结束时，调用注销函数*/
	ret = system("arp -s 136.136.136.136 88:88:88:88:88:88");/*添加MAC与IP的静态映射规则*/
	usleep(20000);
	return nm_connect();
}

void distroy_hw_NetMagic08(void)
{
	nm_release();
}

u64 NetMagic08_reg_rd(u64 regaddr)
{
	struct nm_packet *pkt = (struct nm_packet *)malloc(sizeof(struct nm_packet));
	struct recv_pkt *pkt2 = (struct recv_pkt *)malloc(sizeof(struct recv_pkt));
	int len = 0;
	u64 regvalue = 0;
	pthread_t tid;
	
	pkt->nm.count = 1;
	pkt->nm.type = NM_REG_RD;
	pkt->regaddr = htobe64(regaddr);
	if(pthread_create(&tid, NULL,send_thread, (void *)pkt))
	{
		FAST_ERR("Send NMAC RD PKT Failed!\n");
	}
	if(recv_nm_pkt(pkt2,&len) && pkt2->nh.nm.type == NM_RD_RPL && pkt2->nh.regaddr == htobe64(regaddr))
	{
			regvalue = be64toh(pkt2->nh.regvalue);
	}
	free(pkt2);
	return regvalue;
}


void NetMagic08_reg_wr(u64 regaddr,u64 regvalue)
{
	struct nm_packet *pkt = (struct nm_packet *)malloc(sizeof(struct nm_packet));
	struct recv_pkt *pkt2 = (struct recv_pkt *)malloc(sizeof(struct recv_pkt));
	int len = 0;
	pthread_t tid;

	pkt->nm.count = 1;
	pkt->nm.type = NM_REG_WR;
	pkt->regaddr = htobe64(regaddr);
	pkt->regvalue = htobe64(regvalue);
	if(pthread_create(&tid, NULL,send_thread, (void *)pkt))
	{
		FAST_ERR("Send NMAC WR PKT Failed!\n");
	}
	if(recv_nm_pkt(pkt2,&len) && pkt2->nh.nm.type == NM_WR_RPL)// && pkt2->nh.regaddr == htobe64(regaddr))
	{
		free(pkt2);
		return;
	}
	FAST_ERR("WR REG ERR!\n");
}
#else

#define REG *(volatile u64 *)
u64 OBX_BASE_ADDR = 0x90980000;/*OpenBox*/
//u64 OBX_BASE_ADDR = 0xF7280000;/*OpenBox on i7*/
//u64 OBX_BASE_ADDR = 0xF7D00000;/*NetMagic Pro*/
u64 OBX_REG_LEN = 512*1024;

void *npe_base = NULL;
int fm = 0;

void distroy_hw_OpenBox(void)
{
	if(npe_base != NULL)
	{
		munmap(npe_base,OBX_REG_LEN);
		close(fm);
	}
}

void __distroy_hw_OpenBox(int argc)
{
	distroy_hw_OpenBox();
}

#if 0
#include <pci/pci.h>
#include <pciaccess.h>


int pcie_config_space_read(int vendor_id, int device_id)
{
	
	struct pci_access * myaccess;
	struct pci_dev * mydev;
	int i = 0;

	myaccess = pci_alloc();
	pci_init(myaccess);
	pci_scan_bus(myaccess);		

	for (mydev = myaccess->devices; mydev; mydev = mydev->next)
	{
		pci_fill_info(mydev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);
		if(mydev->vendor_id == vendor_id && mydev->device_id == device_id)
		{
			OBX_BASE_ADDR = 0;
			for (i = 16; i < 20; i++)
			{
				OBX_BASE_ADDR |= pci_read_byte(mydev, i);
				OBX_BASE_ADDR <<= 8;
			}
			OBX_REG_LEN = get_pcie_len();
		}
	}
out:
	pci_free_dev(mydev);
	pci_cleanup(myaccess);
	return 0;
}
#endif

u64 get_pcie_len(void)
{
	int i = 0;
	for(;i<32;i++)
	{
		if(OBX_REG_LEN & (1<<i)) break;
	}
	return 1<<i;
}

int pcie_config_OpenBox(int vendor_id, int device_id)
{
	char cmd[256]={0};
	char value[1024] = {0};
	int len = 0;
	FILE *stream = NULL;
	u32 a=0,b=0,c=0,d=0;

	/*读PCIe配置空间基地址*/
	sprintf(cmd,"%s%X:%X%s","lspci -d ",vendor_id,device_id," -v|grep Memory");
	//sprintf(cmd,"%s%X:*%s","lspci -d ",vendor_id," -v|grep Memory");
	stream = popen(cmd,"r");
	len = fread(value,sizeof(char),sizeof(value),stream);
	sscanf(value,"%s at %lx %s",cmd,&OBX_BASE_ADDR,value);
	pclose (stream);
#if 0

	/*PCIe空间写0后读其长度*/
	sprintf(cmd,"%s%X:%X%s","setpci -d ",vendor_id,device_id," 10.L=FFFFFFFF");
	//sprintf(cmd,"%s%X:*%s","setpci -d ",vendor_id," 10.L=FFFFFFFF");
	len = system(cmd);

	memset(cmd,0,sizeof(cmd));
	memset(value,0,sizeof(value));
	sprintf(cmd,"%s%X:%X%s","lspci -d ",vendor_id,device_id," -x|grep 10:");
	//sprintf(cmd,"%s%X:*%s","lspci -d ",vendor_id," -x|grep 10:");
	stream = popen(cmd,"r");
	len = fread(value,sizeof(char),sizeof(value),stream);
	sscanf(value,"10: %x %x %x %x %s",&d,&c,&b,&a,cmd);
	OBX_REG_LEN = (a<<24)|(b<<16)|(c<<8)|(d);
	pclose (stream);

	/*将配置空间地址重写回BAR空间*/
	sprintf(cmd,"%s%X:%X%s=%lX","setpci -d ",vendor_id,device_id," 10.L",OBX_BASE_ADDR);
	//sprintf(cmd,"%s%X:*%s=%lX","setpci -d ",vendor_id," 10.L",OBX_BASE_ADDR);
	len = system(cmd);	
	OBX_REG_LEN = get_pcie_len();
#endif
	printf("Addr:0x%lX,len:0x%lX,%ld\n",OBX_BASE_ADDR,OBX_REG_LEN,OBX_REG_LEN);	
	return 0;
}

int read_pci_config(void)
{
	return pcie_config_OpenBox(0x8086,0x1502);
}


int init_hw_OpenBox(u64 addr,u64 len)
{	
	if((fm=open("/dev/mem",O_RDWR|O_SYNC)) == -1)
	{
		printf("Open MEM Err!\n");
		exit(0);
	}
	read_pci_config();
	if(addr != 0 && len != 0)
	{
		OBX_BASE_ADDR = addr;
		OBX_REG_LEN = len;
	}
	npe_base = mmap(0,OBX_REG_LEN,PROT_READ|PROT_WRITE,MAP_SHARED,fm,OBX_BASE_ADDR);
	FAST_DBG("REG Version:%s,OpenBox HW Version:%lX\n",REG_VERSION,fast_reg_rd(FAST_HW_REG_VERSION));
	signal(SIGINT,__distroy_hw_OpenBox);//非法结束时，调用注销函数
	return 0;
}
#endif

int fast_init_hw(u64 addr,u64 len)
{	
#ifdef XDL_DEBUG
	return 0;
#elif NetMagic08
	init_hw_NetMagic08(addr,len);
#else
	init_hw_OpenBox(addr,len);
#endif
}

void fast_distroy_hw(void)
{
#ifdef XDL_DEBUG
	return;
#elif NetMagic08
	distroy_hw_NetMagic08();
#else
	distroy_hw_OpenBox();
#endif
}

u64 fast_reg_rd(u64 regaddr)
{
#ifdef XDL_DEBUG
	//printf("reg_rd:%lX = %X\n",regaddr,0x0923);
	return 0x0923;
#elif NetMagic08
	return NetMagic08_reg_rd(regaddr);
#else
	usleep(200);
	return REG(npe_base + regaddr);
#endif
}

void fast_reg_wr(u64 regaddr,u64 regvalue)
{	
#ifdef XDL_DEBUG
	printf("reg_wr:%lX = %lX\n",regaddr,regvalue);
	return;
#elif NetMagic08
	NetMagic08_reg_wr(regaddr,regvalue);
#else
	REG(npe_base + regaddr) = regvalue;
	usleep(200);
#endif
}
