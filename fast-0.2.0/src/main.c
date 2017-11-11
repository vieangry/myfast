/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2017 XuDongLai <XuDong0923@163.com>
 * 
 * fast is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * fast is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../include/fast.h"

//UA回调函数，功能为0、1端口相互转发
int callback(struct fast_packet *pkt,int pkt_len)
{
	if(pkt->um.inport == 0)
	{
		pkt->um.outport = 1;//指定报文输出端口
		pkt->um.dstmid = 5;//将目的模块id号置为5,直接输出
		pkt->um.pktdst = 0;//将输出方向置为0,即输出到端口
		fast_ua_send(pkt,pkt_len);//发送报文往硬件
	}else
	{
		pkt->um.outport = 0;
		pkt->um.dstmid = 5;
		pkt->um.pktdst = 0;
		fast_ua_send(pkt,pkt_len);
	}
	return 0;
}

void ua_init(void)
{
	int ret = 0;
	if((ret=fast_ua_init(129,callback)))//UA模块实例化(输入参数1:接收模块ID号,输入参数2:接收报文的回调处理函数)
	{
		perror("fast_ua_init!\n");
		exit (ret);//如果初始化失败,则需要打印失败信息,并将程序结束退出!
	}
}



void rule_config(void)
{
	int i = 0;
	struct fast_rule rule[8] = {{0},{0},{0},{0},{0},{0},{0},{0}};//初始化八条全空的规则

	//i=0,初始化第0条规则
	//给规则的各字段赋值
//----------写入以太网头部相关字段
	*(u64 *)rule[i].key.dmac = htole64(0x0023CD76631A);//MAC=00:23:CD:76:63:1A
	*(u64 *)rule[i].key.smac = htole64(0x002185C52B8F);//MAC=00:21:85:C5:2B:8F
	rule[i].key.tci = htole16(0x4500);
	rule[i].key.type = htole16(0x0800);
	rule[i].key.tos = 0x32;
	rule[i].key.ttl = 0x35;
	rule[i].key.proto = 0x6;//0x1:ICMP,0x6:TCP,0x11:UDP
//----------写入ipv4协议相关字段，ipv4和ipv6协议不能在同一条规则中生效，会相互覆盖	
	rule[i].key.ipv4.src = htole32(0xC0A80107);//IP=192.168.1.7
	rule[i].key.ipv4.dst = htole32(0xC0A80108);//IP=192.168.1.8
	rule[i].key.ipv4.tp.sport = htole16(0x1388);//Sport = 5000
	rule[i].key.ipv4.tp.dport = htole16(0x50);//Dport = 80 
//----------写入ipv6协议相关字段，ipv6和ipv4协议不能在同一条规则中生效，会相互覆盖		
//--------------------Src_ipv6_addr = 2001:250:4401:2000:6254:7e81:ed31:1	
//	*(u64 *)rule[i].key.ipv6.src.__in6_u.__u6_addr8 =  htole64(0x62547e81ed310001);
//	*(u64 *)(&rule[i].key.ipv6.src.__in6_u.__u6_addr8[8]) =  htole64(0x2001025044012000);

//-------------------Dst_ipv6_addr = 2400:dd01:1034:e00:913f:9bda:7e99:50
//	*(u64 *)rule[i].key.ipv6.dst.__in6_u.__u6_addr8 =  htole64(0x913f9bda7e990050);
//	*(u64 *)(&rule[i].key.ipv6.dst.__in6_u.__u6_addr8[8]) =  htole64(0x2400dd0110340e00);//0x000000000100010b

	rule[i].key.port = 0x0;//写入物理端口号
	rule[i].priority =0xE;//写入第i条规则的优先级，数值越大，优先级越高
	rule[i].action = ACTION_PORT<<28|0x1;//动作字段的涵义请参考fast_type.h，此处位转发往1号端口
	rule[i].md5[0] = i + 1;//写入md5字段，防止规则重复添加，应等于非零值
	//给规则对应字段设置掩码，掩码为1表示使用，为0表示忽略
	//*(u64 *)rule[i].mask.dmac = 0xFFFFFFFFFFFFL;
	//*(u64 *)rule[i].mask.smac = 0xFFFFFFFFFFFFL;
	//rule[i].mask.tag = 0;//0xFFFFFFFF;
	//rule[i].mask.type = 0xFFFF;
	//rule[i].mask.tos = 0xFF;
	//rule[i].mask.ttl = 0xFF;
	//rule[i].mask.proto = 0xFF;//0x1:ICMP,0x6:TCP,0x11:UDP
	//rule[i].mask.ipv4.src = 0xFFFFFFFF;
	//rule[i].mask.ipv4.dst = 0xFFFFFFFF;
    //*(u64 *)rule[i].mask.ipv6.src.__in6_u.__u6_addr8 = htole64(0xFFFFFFFFFFFFFFFF);
    //*(u64 *)(&rule[i].mask.ipv6.src.__in6_u.__u6_addr8[8]) = htole64(0xFFFFFFFFFFFFFFFF);
    //*(u64 *)rule[i].mask.ipv6.dst.__in6_u.__u6_addr8 = htole64(0xFFFFFFFFFFFFFFFF);
    //*(u64 *)(&rule[i].mask.ipv6.dst.__in6_u.__u6_addr8[8]) = htole64(0xFFFFFFFFFFFFFFFF);
	//rule[i].mask.sport = 0xFFFF;
	//rule[i].mask.dport = 0xFFFF;
	//rule[i].mask.port = 0xF;
	
	fast_add_rule(&rule[i]); //添加硬件规则

	print_hw_rule(); //打印硬件中的规则

}


int main(int argc,char* argv[])
{
	int ret = 0;
	int i = 0;

//------------流表部分------------
	fast_init_hw(0,0); //初始化硬件
	init_rule(ACTION_SET_MID << 28 | 129); //初始化硬件流表空间
	rule_config(); //写入规则
//----------UA部分-----------
//	ua_init();//UA模块初始化
//	fast_ua_recv();//启动线程接收分派给UA进程的报文
//	while(1){sleep(9999);}//主进程进入循环休眠中,数据处理主要在回调函数
	fast_distroy_hw();  //销毁硬件资源
	return (0);
}
