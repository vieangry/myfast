/***************************************************************************
 *            main_libofp.c
 *
 *  2017/02/27 09:51:58 星期一
 *  Copyright  2017  XuDongLai
 *  <XuDongLai0923@163.com>
 ****************************************************************************/
/*
 * main_libofp.c
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
#include "main_libofp.h"
#include "md5.h"

#define OFP_MAX_RULE_CNT FAST_RULE_CNT+1

MD5_CTX  md5;
u8 md5_value[16] = {0};
u64 *flow_stats_addr[OFP_MAX_RULE_CNT] = {0};/*用于存储控制器下发的每条flow_mod消息所对应的flow_stats消息地址*/
struct timeval flow_stats_time[OFP_MAX_RULE_CNT] = {0};/*用于记录流表下发的时间,后面用来统计流表时长*/
int ofpfd;					/*OpenFlow Socket 操作句柄*/
struct timeval start_tv;	/*系统开始运行时间*/
int ip_type = 4;			/*默认使用IPv4的地址*/
char controller_ip[256] = {0};	/*OpenFlow 控制器IP地址,可以IPv6,也可以IPv6,地址由运行参数(-4|-6)输入,由此变量保存*/
u32 controller_port = 6653; /*默认floodlight,6633:OpenDayLight*/
char port_list_info[256] = {0};   /*系统运行端口列表,由运行参数(-i)输入,由此变量保存*/
struct nms_ports_info nmps; /*系统端口详细信息,保存端口名称,状态,统计计数,本端口IP,网关IP,网关MAC,本机MAC,libpcap句柄,libnet句柄,发送互斥锁*/
static enum ofperr handle_openflow(struct ofp_buffer *ofpbuf,int len);/*OpenFlow协议处理函数*/
static u8 thread_idx = 1;   /*启动线程号标识,用来控制每个线程打印输出的颜色*/



/*64位主机序转网络序*/
static inline uint64_t
htonll(uint64_t n)
{
    return htonl(1) == 1 ? n : (((uint64_t)htonl(n)) << 32) | htonl(n >> 32);
}

/*64位网络序转主机序*/
static inline uint64_t
ntohll(uint64_t n)
{
    return htonl(1) == 1 ? n : (((uint64_t)ntohl(n)) << 32) | ntohl(n >> 32);
}



/*打印输出报文信息*/
void pkt_print(u8 *pkt,int pkt_len)
{
	int i=0;

	return;
	printf("-----------------------***OFP PACKET***-----------------------\n");
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
	printf("-----------------------***OFP PACKET***-----------------------\n\n");
}


void build_ofp_header(struct ofp_header *ofpbuf_header,uint16_t len,uint8_t type,uint32_t xid)
{
	SHOW_FUN(0);
	ofpbuf_header->version = OFP13_VERSION;
	ofpbuf_header->length = htons(len);	
	LOG_DBG("ofpbuf_header->length=%d\n",ntohs(ofpbuf_header->length));
	ofpbuf_header->type = type;
	ofpbuf_header->xid = xid;
	SHOW_FUN(1);
}

u8 *build_reply_ofpbuf(uint8_t type,uint32_t xid,uint16_t len)
{
	SHOW_FUN(0);	
	struct ofp_header *reply = (struct ofp_header *)malloc(len);
	memset((u8 *)reply,0,len);
	build_ofp_header(reply,len,type,xid);
	LOG_DBG("ofpbuf_reply,malloc:%p,type:%d,len:%d\n",reply,type,len);
	SHOW_FUN(1);
	return (u8 *)reply;
}

void send_openflow_message(struct ofp_buffer *ofpmsg,int len)
{
	SHOW_FUN(0);
	LOG_DBG("ofp_buffer.type=%d,len=%d,fd:%d\n",ofpmsg->header.type,len,ofpfd);
	if(write(ofpfd, ofpmsg,len) == -1)
	{
		LOG_ERR("Send of to Controller Faild!\n");		
	}	
	pkt_print((u8 *)ofpmsg,htons(ofpmsg->header.length));
	LOG_DBG("  free:%p,type:%d\n",ofpmsg,ofpmsg->header.type);
	free(ofpmsg);
	SHOW_FUN(1);
}

void send_hello_message()
{
	struct ofp_header *ofp_header_hello = (struct ofp_header *)build_reply_ofpbuf(OFPT_HELLO,2,8);

	SHOW_FUN(0);
	send_openflow_message((struct ofp_buffer *)ofp_header_hello,sizeof(struct ofp_header));		
	LOG_DBG("Send_HELLO_MESSAGE:\t len = %04x\n", ofp_header_hello->length);	
	SHOW_FUN(1);
}



void send_packet_in_message_863_ok(u32 in_port,u8 *pkt6,int len)
{
	SHOW_FUN(0);
	LOG_DBG("\n\n++++++++++++++SEND PACKET IN+++++++++++++++++++++\n\n");
	int reply_len = sizeof(struct ofp_header) + sizeof(struct ofp_packet_in) + sizeof(struct ofp_oxm) * 4 + (4 + 16 + 16 + 16) + 2 + len;
	int oxm_oft = sizeof(struct ofp_packet_in);
	struct ofp_buffer *ofpbuf_reply = 
		(struct ofp_buffer *)build_reply_ofpbuf(OFPT_PACKET_IN,0x30,reply_len);
	struct ofp_packet_in *send_packet_in= (struct ofp_packet_in *)ofpbuf_reply->data;
	u8 *value = NULL;

	send_packet_in->buffer_id = htonl(0xFFFFFFFF);
	send_packet_in->total_len = htons(reply_len);
	send_packet_in->reason = OFPR_NO_MATCH;//OFPR_ACTION->controller;//OFPR_NO_MATCH->controller->packet_out;
	send_packet_in->table_id = 0;
	send_packet_in->cookie = htonll(0x00);
	send_packet_in->match.length = htons(sizeof(struct ofp_oxm) * 4 + (4 + 16 + 16 + 16) + 4);//All Match and pad len
	send_packet_in->match.type = htons(OFPMT_OXM);

	//---------------------------OXM IN PORT-------------------------------------
	struct ofp_oxm *oxm = (struct ofp_oxm *)&ofpbuf_reply->data[oxm_oft];
	oxm->classname = htons(OFPXMC_OPENFLOW_BASIC);
	oxm->filed = OFPXMT_OFB_IN_PORT;
	oxm->has_mask = 0;//False	
	oxm->length = 4;	
	
	oxm_oft += sizeof(struct ofp_oxm);
	value = (u8 *)&ofpbuf_reply->data[oxm_oft];
	*((u32 *)value) = htonl(in_port);	

	//----------------------------OXM IPV6 SRC-----------------------------------
	oxm_oft += oxm->length;
	oxm = (struct ofp_oxm *)&ofpbuf_reply->data[oxm_oft];
	oxm->classname = htons(OFPXMC_OPENFLOW_BASIC);
	oxm->filed = OFPXMT_OFB_IPV6_SRC;
	oxm->has_mask = 0;//False
	oxm->length = 16;
	
	oxm_oft += sizeof(struct ofp_oxm);
	value = (u8 *)&ofpbuf_reply->data[oxm_oft];
	memset((u8 *)value,0xA,16);
	
	//-----------------------------OXM IPV6 DST----------------------------------
	oxm_oft += oxm->length;	
	oxm = (struct ofp_oxm *)&ofpbuf_reply->data[oxm_oft];
	oxm->classname = htons(OFPXMC_OPENFLOW_BASIC);
	oxm->filed = OFPXMT_OFB_IPV6_DST;
	oxm->length = 16;

	oxm_oft += sizeof(struct ofp_oxm);
	value = (u8 *)&ofpbuf_reply->data[oxm_oft];
	memset((u8 *)value,0xB,16);

	//-----------------------------OXM IPV6 RLOC---------------------------------
	oxm_oft += oxm->length;	
	oxm = (struct ofp_oxm *)&ofpbuf_reply->data[oxm_oft];
	oxm->classname = htons(OFPXMC_OPENFLOW_BASIC);
	oxm->filed = OFPXMT_OFB_IPV6_DST;//OFPXMT_OFB_IPV6_ND_TARGET;
	oxm->length = 16;

	oxm_oft += sizeof(struct ofp_oxm);
	value = (u8 *)&ofpbuf_reply->data[oxm_oft];
	memset((u8 *)value,0xC,16);

	//-----------------------------PKT DATA--------------------------------------
	oxm_oft += oxm->length + 2;
	memcpy((u8 *)&ofpbuf_reply->data[oxm_oft],pkt6,len);	
	
	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);
}

void send_packet_in_message_863(u32 in_port,u8 *pkt6,int len)//确定版本:in_port,eth_type,src_rloc,ipv6_packet
{		
	int reply_len = sizeof(struct ofp_header) + sizeof(struct ofp_packet_in) + sizeof(struct ofp_oxm) * 3 + (4 + 2 + 16) + 4 + len;
	int oxm_oft = sizeof(struct ofp_packet_in);
	struct ofp_buffer *ofpbuf_reply = 
		(struct ofp_buffer *)build_reply_ofpbuf(OFPT_PACKET_IN,0x30,reply_len);//0x30为XID
	struct ofp_packet_in *send_packet_in= (struct ofp_packet_in *)ofpbuf_reply->data;
	struct eth_header *eth = (struct eth_header *)pkt6;
	u8 *value = NULL;

	SHOW_FUN(0);
	send_packet_in->buffer_id = htonl(0xFFFFFFFF);
	send_packet_in->total_len = htons(reply_len);
	send_packet_in->reason = OFPR_NO_MATCH;//OFPR_ACTION->controller;//OFPR_NO_MATCH->controller->packet_out;
	send_packet_in->table_id = 0;
	send_packet_in->cookie = htonll(0x00);
	send_packet_in->match.length = htons(sizeof(struct ofp_oxm) * 3 + (4 + 2 + 16) + 4);//All Match and pad len
	send_packet_in->match.type = htons(OFPMT_OXM);

	//---------------------------OXM IN PORT-------------------------------------
	struct ofp_oxm *oxm = (struct ofp_oxm *)&ofpbuf_reply->data[oxm_oft];
	oxm->classname = htons(OFPXMC_OPENFLOW_BASIC);
	oxm->filed = OFPXMT_OFB_IN_PORT;
	oxm->has_mask = 0;//False	
	oxm->length = 4;	
	
	oxm_oft += sizeof(struct ofp_oxm);
	value = (u8 *)&ofpbuf_reply->data[oxm_oft];
	*((u32 *)value) = htonl(in_port);	

	//----------------------------OXM ETH TYPE-----------------------------------
	oxm_oft += oxm->length;
	oxm = (struct ofp_oxm *)&ofpbuf_reply->data[oxm_oft];
	oxm->classname = htons(OFPXMC_OPENFLOW_BASIC);
	oxm->filed = OFPXMT_OFB_ETH_TYPE;
	oxm->has_mask = 0;//False
	oxm->length = 2;
	
	oxm_oft += sizeof(struct ofp_oxm);
	value = (u8 *)&ofpbuf_reply->data[oxm_oft];
	*((u16 *)value) = htons(eth->frame);
	
	//----------------------------OXM IPV6 SRC RLOC-----------------------------------
	oxm_oft += oxm->length;
	oxm = (struct ofp_oxm *)&ofpbuf_reply->data[oxm_oft];
	oxm->classname = htons(OFPXMC_OPENFLOW_BASIC);
	oxm->filed = OFPXMT_OFB_IPV6_SRC;
	oxm->has_mask = 0;//False
	oxm->length = 16;
	
	oxm_oft += sizeof(struct ofp_oxm);
	value = (u8 *)&ofpbuf_reply->data[oxm_oft];
	memset((u8 *)value,0xA,16);//填充当前节点XTR的IPv6公网IP地址

	//-----------------------------PKT DATA--------------------------------------

	oxm_oft += oxm->length + 4;
	memcpy((u8 *)&ofpbuf_reply->data[oxm_oft],pkt6,len);	

	//printf("oxm:%d,pkt_len:%d,send_len:%d\n",oxm_oft,len,reply_len);
	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);
}


void send_packet_in_message(int in_port,u8 *pkt,int len,int reason)
{
	int reply_len = sizeof(struct ofp_header)+sizeof(struct ofp_packet_in)+sizeof(struct ofp_oxm)+10+len;
	int oxm_oft = sizeof(struct ofp_packet_in);
	struct ofp_buffer *ofpbuf_reply = 
		(struct ofp_buffer *)build_reply_ofpbuf(OFPT_PACKET_IN,0x30,reply_len);
	struct ofp_packet_in *send_packet_in= (struct ofp_packet_in *)ofpbuf_reply->data;
	u32 *value = NULL;
	
	SHOW_FUN(0);
	send_packet_in->buffer_id = htonl(0xFFFFFFFF);
	send_packet_in->total_len = htons(reply_len);
	send_packet_in->reason = reason;//OFPR_ACTION->controller;//OFPR_NO_MATCH->controller->packet_out;//NO_MATCH会产生一个packet_out报文
	send_packet_in->table_id = 0;
	send_packet_in->cookie = htonll(0x00);
	send_packet_in->match.length = htons(12);
	send_packet_in->match.type = htons(OFPMT_OXM);

	struct ofp_oxm *oxm = (struct ofp_oxm *)&ofpbuf_reply->data[oxm_oft];
	oxm->classname = htons(OFPXMC_OPENFLOW_BASIC);
	oxm->filed = OFPXMT_OFB_IN_PORT;
	oxm->has_mask = 0;//False	
	oxm->length = 4;

	oxm_oft += sizeof(struct ofp_oxm);
	value = (u32 *)&ofpbuf_reply->data[oxm_oft];
	*value = htonl(in_port);
	
	oxm_oft += 4 + 4 + 2;//(*value(4) + pad(4) +pad(2))
	memcpy((u8 *)&ofpbuf_reply->data[oxm_oft],pkt,len);	
	
	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);
}
void send_packet_in_message_meter(int in_port,u8 *pkt,int len)
{
	send_packet_in_message(in_port,pkt,len,OFPR_ACTION);//OFPR_ACTION->controller;//OFPR_NO_MATCH->controller->packet_out;//NO_MATCH会产生一个packet_out报文
}
void send_packet_in_message_lldp(int in_port,u8 *pkt,int len)
{		
	send_packet_in_message(in_port,pkt,len,OFPR_ACTION);//OFPR_ACTION->controller;//OFPR_NO_MATCH->controller->packet_out;//NO_MATCH会产生一个packet_out报文
}

void send_packet_in_message_normal(u32 in_port,u8 *pkt,int len)
{
	send_packet_in_message(in_port,pkt,len,OFPR_NO_MATCH);//OFPR_ACTION->controller;//OFPR_NO_MATCH->controller->packet_out;//NO_MATCH会产生一个packet_out报文
}


void open_openflow_connect(char *controller_ip)
{
	SHOW_FUN(0);
	if(ip_type == 6)/*IPv6*/
	{
		struct sockaddr_in6 controller_addr;

		bzero(&controller_addr,sizeof(controller_addr));
		controller_addr.sin6_family = AF_INET6;
		inet_pton(AF_INET6,controller_ip,&controller_addr.sin6_addr);
		controller_addr.sin6_port = htons(controller_port);
		if((ofpfd = socket(AF_INET6,SOCK_STREAM,0)) == -1)
		{
			LOG_ERR("Create socket to controller error!\n");
		}
		if(connect(ofpfd,(struct sockaddr *)&controller_addr,sizeof(controller_addr)))
		{
			LOG_ERR("Connect controller [ %s:%d ] error!\n",controller_ip,controller_port);
		}
		else
		{
			PRINT("Connect to SDN Controller [ %s:%d ] OK!\n",controller_ip,controller_port);
		}
	}
	else
	{
		struct sockaddr_in controller_addr;

		SHOW_FUN(0);
		bzero(&controller_addr,sizeof(controller_addr));
		controller_addr.sin_family = AF_INET;
		inet_pton(AF_INET,controller_ip,&controller_addr.sin_addr);
		controller_addr.sin_port=htons(controller_port);
		if((ofpfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
			LOG_ERR("Create socket to controller error!\n");
		}
		if(connect(ofpfd,(struct sockaddr *)&controller_addr,sizeof(controller_addr)))
		{
			LOG_ERR("Connect controller [ %s:%d ] error!\n",controller_ip,controller_port);
		}
		else
		{
			PRINT("Connect to SDN Controller [ %s:%d ] OK!\n",controller_ip,controller_port);
		}
	}
	SHOW_FUN(1);
}


void close_openflow_connect()
{
	SHOW_FUN(0);
	close(ofpfd);
	SHOW_FUN(1);
}


static enum ofperr
handle_hello(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);	
	if(ofpbuf->header.version==OFP13_VERSION)
	{
		return 0;
	}
	else
	{
		return OFPERR_TEST;
	}	
	SHOW_FUN(1);  
}


static enum ofperr
handle_error(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));	
	SHOW_FUN(1);
	
	return 0;
}


static enum ofperr
handle_echo_request(struct ofp_buffer *ofpbuf)
{
	int reply_len = sizeof(struct ofp_header);	
	struct ofp_buffer *ofpbuf_reply = 
		(struct ofp_buffer *)build_reply_ofpbuf(OFPT_ECHO_REPLY,ofpbuf->header.xid,reply_len);

	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);

	return 0;
}


static enum ofperr
handle_experimenter(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);
	
	SHOW_FUN(1);
    	return 0;
}



static enum ofperr
handle_features_request(struct ofp_buffer *ofpbuf)
{
	int feature_reply_len = sizeof(struct ofp_switch_features)+sizeof(struct ofp_header);	
	struct ofp_buffer *ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_FEATURES_REPLY,
		ofpbuf->header.xid,feature_reply_len);
	struct ofp_switch_features *feature_reply_msg =(struct ofp_switch_features *)ofpbuf_reply->data;

	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	/*feature_reply_body*/
	feature_reply_msg->datapath_id = htonll(*((uint64 *)&nmps.ports[0].state.hw_addr));
	feature_reply_msg->n_buffers = 0x0;//0x100;
	feature_reply_msg->n_tables = 0x01;/*硬件只有一个表*/
	feature_reply_msg->auxiliary_id = 0;
	feature_reply_msg->capabilities = htonl(0x0000004f);
	feature_reply_msg->reserved = htonl(0x00000000);

	send_openflow_message(ofpbuf_reply,feature_reply_len);
	SHOW_FUN(1);

	return 0;
}


static enum ofperr
handle_get_config_request(struct ofp_buffer *ofpbuf)
{
	int reply_len = sizeof(struct ofp_switch_config)+sizeof(struct ofp_header);	
	struct ofp_buffer *ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_GET_CONFIG_REPLY,
		ofpbuf->header.xid,reply_len);
	struct ofp_switch_config *switch_config_reply =(struct ofp_switch_config *)ofpbuf_reply->data;

	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	/*ofp_switch_config_body*/
	switch_config_reply->flags = htons(0x0000);
	switch_config_reply->miss_send_len = htons(0xffff);

	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);

	return 0;
}



static enum ofperr
handle_set_config(struct ofp_buffer *ofpbuf,int len)
{
	int config_reply_len = sizeof(struct ofp_switch_config)+sizeof(struct ofp_header);

	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	LOG_DBG("do_set_config\n");
	SHOW_FUN(1);
	return 0;
}

void send_eth_packet_out(u32 inport,u32 outport,struct eth_header *eth,int len,int hit_idx)
{
	u8 proto = 0;
	int send_len = 0,new_len = len;
	u16 frame_type = 0;
	libnet_ptag_t eth_tag = 0;
	libnet_t *libnet = nmps.ports[outport].net;
	
	pthread_mutex_lock(&nmps.ports[outport].port_send_mutex);
	eth_tag = libnet_build_ethernet(
	          eth->dmac,eth->smac,htons(eth->frame),	          
	          (char *)(eth+1),/*跳到以太网后面开始发送*/
	          len-sizeof(struct eth_header),libnet,eth_tag);
	send_len = libnet_write(libnet);
	libnet_clear_packet(libnet);
	pthread_mutex_unlock(&nmps.ports[outport].port_send_mutex);
	
	if(send_len == len)
	{
		FAST_DBG("PKT_OUT-%s %s [ 0x%04X ]/[ %d ] Packet len:%d OK! [ in:0x%08X ]\n",
		       (frame_type==0x0800) ? "4" : (frame_type==0x86DD) ? "6": "X",
		       nmps.ports[outport].name,frame_type,proto,len,inport);
	}
	else
	{
		FAST_DBG("*PKT_OUT-%s %s [ 0x%04X ]/[ %d ] Packet len:%d ERROR! [ in:0x%08X ]\n",
		       (frame_type==0x0800) ? "4" : (frame_type==0x86DD) ? "6": "X",
		       nmps.ports[outport].name,frame_type,proto,len,inport);
	}	
}

void nms_exec_action(u32 inport,u32 outport,struct eth_header *eth,int len,int hit_idx)
{
	int i = 0;
	
	SHOW_FUN(0);
	switch(outport)
		{
			case OFPP_CONTROLLER:    //发送到控制器
			{
				LOG_DBG("ACTION PKT_IN %s Send [ 0x%04X ],len:%d\n",nmps.ports[inport].name,ntohs(eth->frame),len);
				send_packet_in_message_normal(inport,(u8 *)eth,len);
				break;
			}
			case OFPP_LOCAL:		//暂时不管
			case OFPP_TABLE:
			case OFPP_NORMAL:
				break;
			case OFPP_FLOOD:		//泛洪					
			case OFPP_ALL:
			case OFPP_ANY:
			case OFPP_MAX:
			{					
				for(i=0;i < nmps.cnt;i++)
				{
					if(i == inport)continue;				
					send_eth_packet_out(inport,i,eth,len,hit_idx);
				}
				break;
			}			
			case OFPP_IN_PORT:		//从入端口输出
			{
				send_eth_packet_out(inport,inport,eth,len,hit_idx);
				break;
			}
			default:
			{
				LOG_DBG("FORWARD inport:0x%08X -> outport:0x%08X\n\n",inport,outport);	
				if(outport < nmps.cnt)
				{
					send_eth_packet_out(inport,outport,eth,len,hit_idx);
				}
				else
				{
					LOG_ERR("Packet_out Send to Port[in:0x%08X] ERR:0x%04X >= 0x%04X\n",inport,outport,nmps.cnt);
				}
				break;
			}				
		}	
	SHOW_FUN(1);
}

static enum ofperr
handle_packet_out(struct ofp_buffer *ofpbuf)
{
	struct ofp_packet_out *out = (struct ofp_packet_out *)ofpbuf;
	struct ofp_action_output *action = (struct ofp_action_output *)&out->actions[0];
	struct eth_header *eth = (struct eth_header *)&ofpbuf->data[sizeof(struct ofp_packet_out) - sizeof(struct ofp_header) + sizeof(struct ofp_action_output)];
	int i = 0,send_len = 0;
	
	SHOW_FUN(0);
	LOG_DBG("Packet Out! in [0x%08X] out[0x%08X]\n",ntohl(out->in_port),ntohl(action->port));
	if(action->type == OFPAT_OUTPUT)
	{		
		send_len = ntohs(ofpbuf->header.length) - sizeof(struct ofp_packet_out) - sizeof(struct ofp_action_output);
		nms_exec_action(ntohl(out->in_port),ntohl(action->port),eth,send_len,-1);/*最后参数-1表示未携带查表命中信息,此处仅做发送*/
	}	
	SHOW_FUN(1);
	
	return 0;
}

void show_oxm_value(struct ofp_oxm *oxm)
{
	int i = 0;
	return;
	for(i=0;i<oxm->length;i++)
	{
		printf("%02X",*((u8 *)(oxm + 1) + i));
	}
	printf("\n");
}


u8 get_phyport_by_logport(u32 logport)
{
	return nmps.ports[logport].phyport;
}


static enum ofperr
handle_flow_mod_add(struct ofp_buffer *ofpbuf)
{
	struct ofp_flow_mod *flow_mod = (struct ofp_flow_mod *)ofpbuf->data;
	int oft_oxm =  0,i = 0;	
	struct ofp_instruction_flow_stats *inst = NULL;
	int padded_match_len = 0;
	struct fast_rule *rule = (struct fast_rule *)malloc(sizeof(struct fast_rule));
	u32 port = 0,tag = 0x8011<<16,tag_mask = 0xFFFF0000,rule_action = 0;/*默认规则是丢弃*/
	u16 vlan_vid = 0,vlan_pcp = 0,new_port = 0;
	u8 ip_dscp = 0,ip_ecn = 0,ip_tos_mask = 0;
	
	SHOW_FUN(0);
	memset(rule,0,sizeof(struct fast_rule));
	rule->cookie = htole64(flow_mod->cookie);
	rule->cookie_mask = htole64(flow_mod->cookie_mask);
	rule->priority = htole32(ntohs(flow_mod->priority));
	
	padded_match_len = ROUND_UP(ntohs(flow_mod->match.length),8);
	if(padded_match_len + sizeof(struct ofp_header) + sizeof(struct ofp_flow_mod) - sizeof(struct ofp_match) == ntohs(ofpbuf->header.length))
	{
		/*只有MATCH字段,不做处理*/
		free(rule);
		return 0;
	}
	
	while(ntohs(flow_mod->match.length) - sizeof(flow_mod->match.type) - sizeof(flow_mod->match.length) > oft_oxm)
	{
        struct ofp_oxm *oxm = (struct ofp_oxm *)&ofpbuf->data[oft_oxm + sizeof(struct ofp_flow_mod) - 4];//match has pad[4]

		switch(oxm->filed)
		{
			case OFPXMT_OFB_IN_PORT:			
				memcpy(&port,oxm +1 , oxm->length);
				new_port = ntohl(port);				
				rule->key.port = new_port;  /*port为4bit,2017/06/01后启用*/
				rule->mask.port=0xF;		/*port为4bit*/	
				LOG_DBG("filed:OFPXMT_OFB_IN_PORT,len:%d,INPORT:",oxm->length);	
				show_oxm_value(oxm);
				
				break;
				
			case OFPXMT_OFB_ETH_DST:
				oxm2rule(&rule->key.dmac[0],oxm+1,oxm->length);
				//memcpy(&rule->key.dmac,oxm +1,oxm->length);				
				//*(u64 *)rule->key.dmac = htole64(*((u64 *)(oxm +1)));
				memset(&rule->mask.dmac,0xFF,oxm->length);				
				LOG_DBG("filed:OFPXMT_OFB_ETH_DST,len:%d,SMAC:",oxm->length);
				show_oxm_value(oxm);
				break;
				
			case OFPXMT_OFB_ETH_SRC:
				oxm2rule(&rule->key.smac[0],oxm+1,oxm->length);
				//memcpy(&rule->key.smac,oxm +1,oxm->length);
				//*(u64 *)rule->key.smac = htole64(*((u64 *)(oxm +1)));
				memset(&rule->mask.smac,0xFF,oxm->length);				
				LOG_DBG("filed:OFPXMT_OFB_ETH_SRC,len:%d,DMAC:",oxm->length);
				show_oxm_value(oxm);
				
				break;
				
			case OFPXMT_OFB_VLAN_VID:
				memcpy(&vlan_vid,oxm +1 , oxm->length);
				tag |= vlan_vid&0xFFF;
				tag_mask |= 0xFFF;
				rule->key.tci = n2rule16(tag);
				rule->mask.tci = n2rule16(tag_mask);
				LOG_DBG("filed:OFPXMT_OFB_VLAN_VID,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				
				break;
				
			case OFPXMT_OFB_VLAN_PCP:
				memcpy(&vlan_pcp,oxm +1 , oxm->length);
				tag |= (vlan_pcp&0xF)<<13;
				tag_mask |= 0xE000;
				rule->key.tci = n2rule16(tag);
				rule->mask.tci = n2rule16(tag_mask);
				LOG_DBG("filed:OFPXMT_OFB_VLAN_PCP,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				
				break;
				
			case OFPXMT_OFB_ETH_TYPE:
				oxm2rule(&rule->key.type,oxm +1 , oxm->length);
				memset(&rule->mask.type,0xFF,oxm->length);				
				LOG_DBG("filed:OFPXMT_OFB_ETH_TYPE,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				
				break;
				
			case OFPXMT_OFB_IP_DSCP:
				memcpy(&ip_dscp,oxm +1 , oxm->length);			
				rule->key.tos |= ip_dscp<<2;
				rule->mask.tos |= 0xFC;
				LOG_DBG("filed:OFPXMT_OFB_IP_DSCP,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				
				break;
					
			case OFPXMT_OFB_IP_ECN:
				memcpy(&ip_ecn,oxm +1 , oxm->length);
				rule->key.tos |= ip_ecn;
				rule->mask.tos |= 0x3;
				LOG_DBG("filed:OFPXMT_OFB_IP_ECN,len:%d,TYPE:\n",oxm->length);	
				show_oxm_value(oxm);
				
				break;	

			case OFPXMT_OFB_IP_PROTO:
				oxm2rule(&rule->key.proto,oxm +1 , oxm->length);
				memset(&rule->mask.proto,0xFF,oxm->length);
				LOG_DBG("filed:OFPXMT_OFB_IP_PROTO,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				
				break;				
				
			case OFPXMT_OFB_IPV4_SRC:
				oxm2rule(&rule->key.ipv4.src,oxm +1 , oxm->length);
				memset(&rule->mask.ipv4.src,0xFF,oxm->length);
				LOG_DBG("filed:OFPXMT_OFB_IPV4_SRC,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				break;

			case OFPXMT_OFB_IPV4_DST:
				oxm2rule(&rule->key.ipv4.dst,oxm +1 , oxm->length);
				memset(&rule->mask.ipv4.dst,0xFF,oxm->length);
				LOG_DBG("filed:OFPXMT_OFB_IPV4_DST,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				break;

			case OFPXMT_OFB_IPV6_SRC:
				oxm2rule(&rule->key.ipv6.src,oxm +1 , oxm->length);
				memset(&rule->mask.ipv6.src,0xFF,oxm->length);
				LOG_DBG("filed:OFPXMT_OFB_IPV6_SRC,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				break;

			case OFPXMT_OFB_IPV6_DST:
				oxm2rule(&rule->key.ipv6.dst,oxm +1 , oxm->length);
				memset(&rule->mask.ipv6.dst,0xFF,oxm->length);
				LOG_DBG("filed:OFPXMT_OFB_IPV6_DST,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				break;

			case OFPXMT_OFB_TCP_SRC:
			case OFPXMT_OFB_UDP_SRC:
				if(rule->key.type == 0x0800)
				{
					oxm2rule(&rule->key.ipv4.tp.sport,oxm +1 , oxm->length);
					memset(&rule->mask.ipv4.tp.sport,0xFF,oxm->length);
				}
				else
				{
					oxm2rule(&rule->key.ipv6.tp.sport,oxm +1 , oxm->length);
					memset(&rule->mask.ipv6.tp.sport,0xFF,oxm->length);
				}
				LOG_DBG("filed:OFPXMT_OFB_TCP/UDP_SRC,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				break;

			case OFPXMT_OFB_TCP_DST:
			case OFPXMT_OFB_UDP_DST:
				if(rule->key.type == 0x0800)
				{
					oxm2rule(&rule->key.ipv4.tp.dport,oxm +1 , oxm->length);
					memset(&rule->mask.ipv4.tp.dport,0xFF,oxm->length);
				}
				else
				{
					oxm2rule(&rule->key.ipv6.tp.dport,oxm +1 , oxm->length);
					memset(&rule->mask.ipv6.tp.dport,0xFF,oxm->length);
				}
				LOG_DBG("filed:OFPXMT_OFB_TCP/UDP_DST,len:%d,TYPE:",oxm->length);	
				show_oxm_value(oxm);
				break;			
			default:
			{
				printf("filed:DEFAULT,HW can not set this filed!\n");
				printf("MATCH FILED: %d[",oxm->filed);
				for(i = 0;i<oxm->length;i++)
				{
					printf("%02X",(u8)ofpbuf->data[oft_oxm + sizeof(struct ofp_flow_mod) + sizeof(struct ofp_oxm) + i]);
				}
				printf("](len:%d)\n",oxm->length);
				///////////////LOG_ERR("unsupported FILED!\n");
				break;
			}
        }
		oft_oxm += sizeof(struct ofp_oxm) + oxm->length;
	}	

	struct ofp_action_output *out = NULL;
	int padded_inst_len = ROUND_UP(sizeof(struct ofp_instruction),8),oft_out = 0;
	int default_rule = 0;
	inst = (struct ofp_instruction_flow_stats *)&flow_mod->pad[sizeof(flow_mod->pad) + padded_match_len];
	LOG_DBG("INST_type:%d,len:%d\n",ntohs(inst->type),ntohs(inst->len));

	/*仅显示*/
	while(ntohs(inst->len) - sizeof(struct ofp_instruction_flow_stats) > oft_out)/*ODL多个ACTION，暂不支持*/
	{
		out = (struct ofp_action_output *)&flow_mod->pad[sizeof(flow_mod->pad) + padded_match_len + padded_inst_len + oft_out];
		//FAST_DBG("ACTIONS->type:%d,port:0x%X\n",ntohs(out->type),ntohl(out->port));
		oft_out += sizeof(struct ofp_action_output);
	}

	if (padded_inst_len == ntohs(inst->len))/*不带具体动作，做为丢弃处理*/
	{		
		LOG_DBG("DROP Action!\n");	
		rule_action = 0;/*丢弃动作为0表示*/
	}
	else
	{
		out = (struct ofp_action_output *)&flow_mod->pad[sizeof(flow_mod->pad) + padded_match_len + padded_inst_len];
		LOG_DBG("FLOW_MOD->Output:0x%04X,len:%d,max_len:0x%04X\n",ntohl(out->port),ntohs(out->len),ntohs(out->max_len));
		if(ntohl(out->port) == OFPP_CONTROLLER && oft_oxm == 0)/*送控制器，且没有match域*/
		{
			default_rule = 1;
			rule_action = ACTION_SET_MID<<28|FAST_DMID_PROTO_STACK;/*送协议栈处理,交控制器处理，对应端口号输入队列*/			
		}
		else if(ntohl(out->port) >= 0 && ntohl(out->port) < 10)/*OpenBox平台端口数量范围*/
		{
			rule_action = (ACTION_PORT<<28)|get_phyport_by_logport(ntohl(out->port));//将逻辑端口转换为硬件物理端口，支持用户随机输入接口
		}
		else
		{
			rule_action = ACTION_SET_MID<<28|FAST_DMID_PROTO_STACK;/*其他，默认送控制器*/
		}
	}

	struct ofp_flow_stats *stats = (struct ofp_flow_stats *)malloc(ntohs(ofpbuf->header.length));
	struct ofp_instruction_flow_stats *stats_ins = (struct ofp_instruction_flow_stats *)((u8 *)&flow_mod->match + padded_match_len);
	struct timeval tv;
	int idx = OFP_MAX_RULE_CNT - 1;

	gettimeofday(&tv,NULL);
	memset(stats,0,ntohs(ofpbuf->header.length));			
	/*先保存此flow_mod的流表信息,供控制器查询返回交换机的流表配置信息*/
	stats->length = htons(sizeof(struct ofp_flow_stats) - sizeof(struct ofp_match) +  padded_match_len + ntohs(stats_ins->len));
	stats->table_id = 0;/*我们只有硬件一个流表,ID为0*/
	stats->duration_sec = 0;
	stats->duration_nsec = 0;
	stats->priority = flow_mod->priority;
	stats->idle_timeout = flow_mod->idle_timeout;
	stats->hard_timeout = flow_mod->hard_timeout;
	stats->flags = flow_mod->flags;
	stats->cookie = flow_mod->cookie;
	stats->packet_count = 0;
	stats->byte_count = 0;
	/*复制flow_mod的match字段*/
	memcpy(&stats->match,&flow_mod->match,padded_match_len);
	/*复制flow_mod的ins字段*/
	memcpy((u8 *)&stats->match + padded_match_len,stats_ins,ntohs(stats_ins->len));

	if(default_rule && flow_stats_addr[idx] == NULL)
	{		
		fast_reg_wr(FAST_ACTION_REG_ADDR + FAST_DEFAULT_RULE_ADDR,rule_action);
		flow_stats_addr[idx] = (u64 *)stats;/*存储流表记录地址*/
		flow_stats_time[idx] = tv;/*记录当前流表下发的时间点*/
		PRINT("Set Default Action!\n");
		free(rule);
		return;
	}

	MD5Init(&md5);/*保证每次计算时都是原始状态开始*/
	MD5Update(&md5,(unsigned char *)rule,sizeof(rule->key) + sizeof (rule->mask) + sizeof(rule->priority));
	MD5Final(&md5,(unsigned char *)&rule->md5);
	if((idx=rule_exists(rule)) == -1 )/*如果规则不存在,才做添加*/
	{			
		if(flow_mod->command == OFPFC_ADD)
		{
readd_flow:
			rule->flow_stats_len = ntohs(stats->length);
			rule->flow_stats_info = (u64 *)stats;

			/*添加硬件流表规则*/		
			rule->action = rule_action;	
			rule->key.port = get_phyport_by_logport(rule->key.port);//将逻辑端口转换为硬件物理端口，支持用户随机输入接口			
			if((idx = fast_add_rule(rule)) >=0)
			{			
				flow_stats_addr[idx] = (u64 *)stats;/*存储流表记录地址*/
				flow_stats_time[idx] = tv;/*记录当前流表下发的时间点*/
			}
			else
			{			
				print_sw_rule();/*打印当前所有规则*/
				LOG_ERR("ADD RULE ERROR! OVERFLOW!!!\n");
			}
			PRINT("ADD HW NEW(%d) RULE!\n",idx);
		}
		else
		{			
			if(oft_oxm > 0 && (flow_mod->command == OFPFC_MODIFY_STRICT || flow_mod->command == OFPFC_MODIFY))/*有匹配字段，可能是软件重启，丢失流表，可重新添加*/
			{
				PRINT("NO ADD BUT NO RULE:%d,oft_oxm:%d,READD\n",flow_mod->command,oft_oxm);
				goto readd_flow;
			}
			else/*可能是删除一条不存在的规则*/
			{
				PRINT("NO ADD BUT NO RULE:%d,oft_oxm:%d,DO NOTHING!\n",flow_mod->command,oft_oxm);
				free(stats);
			}
		}
	}	
	else
	{
		if(flow_mod->command == OFPFC_DELETE_STRICT)
		{			
			free(flow_stats_addr[idx]);/*释放存储流表记录地址空间*/
			flow_stats_addr[idx] = NULL;/*删除存储流表记录地址*/
			
			fast_reg_wr(FAST_ACTION_REG_ADDR + idx*8,rule_action);	
			fast_del_rule(idx);
			PRINT("HW RULE EXISTS(%d)! DELETE!\n",idx);
		}
		else if(flow_mod->command == OFPFC_MODIFY_STRICT)
		{
			fast_reg_wr(FAST_ACTION_REG_ADDR + idx*8,rule_action);
			
			free(flow_stats_addr[idx]);/*释放原来存储流表记录地址空间*/
			flow_stats_addr[idx] = (u64 *)stats;/*存储流表记录地址*/
			flow_stats_time[idx] = tv;/*记录当前流表下发的时间点*/
			
			PRINT("HW RULE EXISTS(%d)!UPDATE ACTION!\n",idx);	
		}
		else
		{
			PRINT("HW RULE EXISTS(%d)! command:%d,DO NOTHING\n",idx,flow_mod->command);
			free(stats);
		}
	}
	free(rule);
	SHOW_FUN(1);
	return 0;
}



static enum ofperr
handle_flow_mod_delete(struct ofp_buffer *ofpbuf)
{
	int i = 0;
	struct ofp_flow_mod *flow_mod = (struct ofp_flow_mod *)ofpbuf->data;
	//u32 action = ACTION_SET_MID<<28|FAST_DMID_PROTO_STACK;/*送协议栈处理,交控制器处理*/
	u32 action = 0x0;/*丢弃*/
	if(flow_mod->cookie_mask == 0)/*删除所有规则*/
	{
		PRINT("DELETE ALL RULE!\n");
		for(;i<OFP_MAX_RULE_CNT-1;i++)
		{
			fast_reg_wr(FAST_ACTION_REG_ADDR + i*8,action);/*将所有规则的动作置为，送控制器处理*/
		}
	}
	else
	{
		for(;i<OFP_MAX_RULE_CNT-1;i++)
		{
			if(flow_stats_addr[i] != NULL)
			{		
				PRINT("DELETE %d RULE!\n",i);
				struct ofp_flow_stats *stats = (struct ofp_flow_stats *)flow_stats_addr[i];
				if(stats->cookie == flow_mod->cookie)
				{
					fast_del_rule(i);/*删除软件规则*/
					fast_reg_wr(FAST_ACTION_REG_ADDR + i*8,action);/*修改硬件action*/
				}
			}
		}
	}
}

static enum ofperr
handle_flow_mod(struct ofp_buffer *ofpbuf)
{
	struct ofp_flow_mod *flow_mod = (struct ofp_flow_mod *)ofpbuf->data;
	switch(flow_mod->command)
	{
		/*添加新规则*/
		case OFPFC_ADD:					
			return handle_flow_mod_add(ofpbuf);

		/*删除匹配的所有规则*/
		case OFPFC_DELETE:
			return handle_flow_mod_delete(ofpbuf);

		/*修改规则*/
		case OFPFC_MODIFY:
			return handle_flow_mod_add(ofpbuf);

		/*精确修改规则，要求严格匹配字段和优先级*/
		case OFPFC_MODIFY_STRICT:
			return handle_flow_mod_add(ofpbuf);

		/*精确修改规则，要求严格匹配字段和优先级*/
		case OFPFC_DELETE_STRICT:
			return handle_flow_mod_add(ofpbuf);
	}
}


static enum ofperr
handle_group_mod(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	SHOW_FUN(1);
	return 0;
}


static enum ofperr
handle_port_mod(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	SHOW_FUN(1);
	return 0;
}


static enum ofperr
handle_table_mod(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	SHOW_FUN(1);
	return 0;
}


/* Similar to strlcpy() from OpenBSD, but it never reads more than 'size - 1'
 * bytes from 'src' and doesn't return anything. */
void magicrouter_strlcpy(char *dst, const char *src, size_t size)
{
    if (size > 0) {
        size_t len = strnlen(src, size - 1);
        memcpy(dst, src, len);
        dst[len] = '\0';
    }
}


static enum ofperr
handle_ofpmp_desc(struct ofp_buffer *ofpbuf)
{	
	int reply_len = sizeof(struct ofp_header)+sizeof(struct ofp_multipart)+sizeof(struct ofp_desc_stats);
	struct ofp_buffer *ofpbuf_reply = 
		(struct ofp_buffer *)build_reply_ofpbuf(OFPT_MULTIPART_REPLY,ofpbuf->header.xid,reply_len);
	struct ofp_multipart *ofpmp_reply = (struct ofp_multipart *)ofpbuf_reply->data;

    static const char *default_mfr_desc = "HuNan XinShi NetWork";
#ifdef NetMagic08
    static const char *default_hw_desc = "OpenBox HW 2017";
    static const char *default_sw_desc = "OpenBox Driver 1.0.0";    
#else
	static const char *default_hw_desc = "NetMagic08 HW 2017";
    static const char *default_sw_desc = "NetMagic08 Driver 1.0.0";  
#endif
	static const char *default_serial_desc = "None";
    static const char *default_dp_desc = "None";
	
	SHOW_FUN(0);
	ofpmp_reply->type = htons(OFPMP_DESC);
	ofpmp_reply->flags = htonl(OFPMP_REPLY_MORE_NO);
    magicrouter_strlcpy(ofpmp_reply->ofpmp_desc[0].mfr_desc, default_mfr_desc,
                sizeof ofpmp_reply->ofpmp_desc[0].mfr_desc);
    magicrouter_strlcpy(ofpmp_reply->ofpmp_desc[0].hw_desc, default_hw_desc,
                sizeof ofpmp_reply->ofpmp_desc[0].hw_desc);
    magicrouter_strlcpy(ofpmp_reply->ofpmp_desc[0].sw_desc, default_sw_desc,
                sizeof ofpmp_reply->ofpmp_desc[0].sw_desc);
    magicrouter_strlcpy(ofpmp_reply->ofpmp_desc[0].serial_num,default_serial_desc,
                sizeof ofpmp_reply->ofpmp_desc[0].serial_num);
    magicrouter_strlcpy(ofpmp_reply->ofpmp_desc[0].dp_desc, default_dp_desc,
                sizeof ofpmp_reply->ofpmp_desc[0].dp_desc);
	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);
    return 0;
}

static enum ofperr
handle_ofpmp_flow_stats(struct ofp_buffer *ofpbuf)
{	
	int i = 0,reply_len = 0,flow_stats_oft;
	struct ofp_flow_stats *ofp_flow_stats = NULL;
	struct ofp_buffer *ofpbuf_reply = NULL;
	struct ofp_multipart *ofpmp_reply = NULL;
	
	
	for(;i<OFP_MAX_RULE_CNT;i++)
	{
		if(flow_stats_addr[i] != NULL)
		{			
			reply_len += ntohs(((struct ofp_flow_stats *)flow_stats_addr[i])->length);
		}
	}
	reply_len += sizeof(struct ofp_header)+ sizeof(struct ofp_multipart);
	ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_MULTIPART_REPLY,
		ofpbuf->header.xid,reply_len);
	ofpmp_reply = (struct ofp_multipart *)ofpbuf_reply->data;

	ofpmp_reply->type = htons(OFPMP_FLOW);
	ofpmp_reply->flags =  htonl(OFPMP_REPLY_MORE_NO);
	
	flow_stats_oft= sizeof(struct ofp_multipart);
	ofp_flow_stats = (struct ofp_flow_stats *)&ofpbuf_reply->data[flow_stats_oft];
	
	for(i=0;i<OFP_MAX_RULE_CNT;i++)
	{
		if(flow_stats_addr[i] != NULL)
		{	
			memcpy(ofp_flow_stats,flow_stats_addr[i],ntohs(((struct ofp_flow_stats *)flow_stats_addr[i])->length));
			flow_stats_oft += ntohs(ofp_flow_stats->length);
			ofp_flow_stats = (struct ofp_flow_stats *)&ofpbuf_reply->data[flow_stats_oft];			
		}
	}
	send_openflow_message(ofpbuf_reply,reply_len);	
    return 0;
}

static enum ofperr
handle_ofpmp_flow_stats_old_OK(struct ofp_buffer *ofpbuf)
{	
	int reply_len = sizeof(struct ofp_header)+ sizeof(struct ofp_multipart)+sizeof(struct ofp_flow_stats)+sizeof(struct ofp_instruction_flow_stats)+sizeof(struct ofp_action_output);
	struct ofp_buffer *ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_MULTIPART_REPLY,
		ofpbuf->header.xid,reply_len);
	struct ofp_multipart *ofpmp_reply = (struct ofp_multipart *)ofpbuf_reply->data;
	int flow_stats_oft= sizeof(struct ofp_multipart);
	struct ofp_flow_stats *ofp_flow_stats = (struct ofp_flow_stats *)&ofpbuf_reply->data[flow_stats_oft];
	struct ofp_flow_stats_request *ofp_flow_stats_request = (struct ofp_flow_stats_request *)&ofpbuf->data[flow_stats_oft];	
	struct timeval tv;

	SHOW_FUN(0);
	ofpmp_reply->type = htons(OFPMP_FLOW);
	ofpmp_reply->flags =  htonl(OFPMP_REPLY_MORE_NO);

	gettimeofday(&tv,NULL);
	ofp_flow_stats->length = htons(sizeof(struct ofp_flow_stats)+sizeof(struct ofp_instruction_flow_stats)+sizeof(struct ofp_action_output));
	ofp_flow_stats->table_id = 0;
	ofp_flow_stats->duration_sec = htonl(tv.tv_sec - start_tv.tv_sec);
	ofp_flow_stats->duration_nsec = htonl(tv.tv_usec - start_tv.tv_usec);
	ofp_flow_stats->priority = htons(0);
	ofp_flow_stats->idle_timeout = htons(0);
	ofp_flow_stats->hard_timeout = htons(0);
	ofp_flow_stats->flags = htons(0);//
	ofp_flow_stats->cookie = htonll(0);
	ofp_flow_stats->packet_count = htonll(12);
	ofp_flow_stats->byte_count = htonll(1033);

	memcpy((u8 *)&ofp_flow_stats->match,(u8 *)&ofp_flow_stats_request->match,sizeof(struct ofp_match));
	ofp_flow_stats->instructions[0].type = htons(OFPIT_APPLY_ACTIONS);
	ofp_flow_stats->instructions[0].len = htons(24);

	ofp_flow_stats->instructions[0].action_output[0].type = htons(OFPAT_OUTPUT);
	ofp_flow_stats->instructions[0].action_output[0].len = htons(sizeof(struct ofp_action_output));
	ofp_flow_stats->instructions[0].action_output[0].port = htonl(0xfffffffd);
	ofp_flow_stats->instructions[0].action_output[0].max_len = htons(0xffff);
	
	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);
    return 0;
}


static enum ofperr
handle_ofpmp_aggregate(struct ofp_buffer *ofpbuf)
{	
	int reply_len = sizeof(struct ofp_header)+ sizeof(struct ofp_multipart)+sizeof(struct ofp_aggregate_stats_reply);
	struct ofp_buffer *ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_MULTIPART_REPLY,
		ofpbuf->header.xid,reply_len);
	struct ofp_multipart *ofpmp_reply = (struct ofp_multipart *)ofpbuf_reply->data;
	int i = 0,flow_count = 0;
	u64 byte_count = 0,packet_count = 0;
	struct timeval tv;
	
	SHOW_FUN(0);
	ofpmp_reply->type = htons(OFPMP_AGGREGATE);
	ofpmp_reply->flags =  htonl(OFPMP_REPLY_MORE_NO);
	gettimeofday(&tv,NULL);
	for(;i<OFP_MAX_RULE_CNT;i++)
	{
		if(flow_stats_addr[i] != NULL)
		{
			/*在此处更新流计数器!!!此消息请求频率比流表请求要高*/
			((struct ofp_flow_stats *)flow_stats_addr[i])->duration_sec = htonl(tv.tv_sec - flow_stats_time[i].tv_sec);
			((struct ofp_flow_stats *)flow_stats_addr[i])->duration_nsec = htonl(tv.tv_usec - flow_stats_time[i].tv_usec);
			((struct ofp_flow_stats *)flow_stats_addr[i])->packet_count = htonll(0x0923);
			((struct ofp_flow_stats *)flow_stats_addr[i])->byte_count = htonll(0x1982);
			
			packet_count += ntohll(((struct ofp_flow_stats *)flow_stats_addr[i])->packet_count);
			byte_count += ntohll(((struct ofp_flow_stats *)flow_stats_addr[i])->byte_count);
			flow_count++;			
		}
	}	
	ofpmp_reply->ofpmp_aggregate_reply[0].packet_count = htonll(packet_count);
	ofpmp_reply->ofpmp_aggregate_reply[0].byte_count = htonll(byte_count);
	ofpmp_reply->ofpmp_aggregate_reply[0].flow_count = htonl(flow_count);

	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);
    return 0;
}

void skipline(FILE *f)   
{   
	int ch;   
	do
	{
		ch = getc(f);
	}while(ch != '\n' && ch != EOF);
}  

char *get_name(char *name,char *buf)
{
	char *t = NULL;
	while((*buf < 'a') || (*buf > 'z')) buf++;

	if((t=strchr(buf,':')))
	{
		memcpy(name,buf,t-buf);
		return t + 1;
	}
	return NULL;
}

void read_port_stats(char ifname[6],struct ofp_port_stats *of_stats)
{
	FILE *dev_file;
	char name[6] = {0};
	char buf[256] = {0};
	char *str = NULL;
	struct netdev_stats stats;
	
	dev_file = fopen("/proc/net/dev","r");
	if(!dev_file)
	{
		LOG_ERR("open /proc/net/dev|%s Error!\n",ifname);
	}
	skipline(dev_file);
	skipline(dev_file);
	
	while(fgets(buf,sizeof(buf),dev_file))
	{
		memset(name,0,sizeof(name));
		str = get_name(name,buf);
		if(str && !strncmp(name,ifname,strlen(ifname)))
		{
			sscanf(str,"%llu%llu%lu%lu%lu%lu%lu%lu%llu%llu%lu%lu%lu%lu%lu%lu",
			&stats.rx_bytes,
			&stats.rx_packets,
			&stats.rx_errors,
			&stats.rx_dropped,
			&stats.rx_fifo_errors,
			&stats.rx_frame_errors,
			&stats.rx_compressed,
			&stats.rx_multicast,
			&stats.tx_bytes,
			&stats.tx_packets,
			&stats.tx_errors,
			&stats.tx_dropped,
			&stats.tx_fifo_errors,
			&stats.collisions,
			&stats.tx_carrier_errors,
			&stats.tx_compressed);
			
			of_stats->rx_bytes 			= htonll(stats.rx_bytes);
			of_stats->rx_packets		= htonll(stats.rx_packets);
			of_stats->rx_errors 		= htonll(stats.rx_errors);
			of_stats->rx_dropped 		= htonll(stats.rx_dropped);			
			of_stats->rx_frame_err 		= htonll(stats.rx_frame_errors);	
			
			of_stats->tx_bytes 			= htonll(stats.tx_bytes);
			of_stats->tx_packets 		= htonll(stats.tx_packets);
			of_stats->tx_errors 		= htonll(stats.tx_errors);
			of_stats->tx_dropped 		= htonll(stats.tx_dropped);			
			of_stats->collisions 		= htonll(stats.collisions);
			
			return ;
		}
	}
	LOG_ERR("read %s stats Error!\n",ifname);
}

static enum ofperr
handle_ofpmp_port_stats(struct ofp_buffer *ofpbuf)
{
	int i = 0;
	int reply_len = sizeof(struct ofp_header)+sizeof(struct ofp_multipart)+ 
		sizeof(struct ofp_port_stats)*nmps.cnt;
	struct ofp_buffer *ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_MULTIPART_REPLY,
		ofpbuf->header.xid,reply_len);
	struct ofp_multipart *ofpmp_reply = (struct ofp_multipart *)ofpbuf_reply->data;
	struct timeval tv;
	
	SHOW_FUN(0);

	ofpmp_reply->type = htons(OFPMP_PORT_STATS);
	ofpmp_reply->flags = htonl(OFPMP_REPLY_MORE_NO);

	for(i=0;i<nmps.cnt;i++){
		gettimeofday(&tv,NULL);
		//read_port_stats(nmps.ports[i].phyport,&nmps.ports[i].stats);/*不实时读取，改由线程间歇性读*/
		ofpmp_reply->ofpmp_port_stats[i] = nmps.ports[i].stats;
		ofpmp_reply->ofpmp_port_stats[i].duration_sec = htonl(start_tv.tv_sec - tv.tv_sec);
		ofpmp_reply->ofpmp_port_stats[i].duration_nsec = htonl(tv.tv_usec);
	}	
	send_openflow_message(ofpbuf_reply,reply_len);	
	SHOW_FUN(1);
	
	return 0;
}



static enum ofperr
handle_ofpmp_table_features(struct ofp_buffer *ofpbuf)
{
	int fp;
	int table_num = 1;
	int read_len;
	int table_features_prop_oft = 0;
	int reply_len = sizeof(struct ofp_header)+sizeof(struct ofp_multipart)+ 8000*table_num;
	struct ofp_table_features *table=NULL;
	
	struct ofp_buffer *ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_MULTIPART_REPLY,
		ofpbuf->header.xid,reply_len);
	struct ofp_multipart *ofpmp_reply = (struct ofp_multipart *)ofpbuf_reply->data;

	SHOW_FUN(0);
	ofpmp_reply->type = htons(OFPMP_TABLE_FEATURES);
	ofpmp_reply->flags = htonl(OFPMP_REPLY_MORE_NO);

	if((fp = open ("table_features",O_RDWR,S_IRUSR))==-1){
		LOG_ERR("faild to read file:table_features!\n");
	}
	
	read_len = read(fp,&ofpmp_reply->ofpmp_table_features[0],8000);
	close(fp);

	LOG_DBG("\ntable_features_len=%d\n\n",read_len);

	ofpbuf_reply->header.length = htons(read_len+sizeof(struct ofp_header)+sizeof(struct ofp_multipart));	
	send_openflow_message(ofpbuf_reply,read_len+sizeof(struct ofp_header)+sizeof(struct ofp_multipart));
	SHOW_FUN(1);
    return 0;
}


#if 0 

static enum ofperr
handle_ofpmp_table_features(struct ofp_buffer *ofpbuf)
{	
	LCX_FUN();
	int table_num = 1;
	int table_features_prop_oft = 0;
	int reply_len = sizeof(struct ofp_header)+sizeof(struct ofp_multipart)+ 4000*table_num;
	struct ofp_buffer *ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_MULTIPART_REPLY,
		ofpbuf->header.xid,reply_len);
	struct ofp_multipart *ofpmp_reply = (struct ofp_multipart *)ofpbuf_reply->data;
	ofpmp_reply->type = htons(OFPMP_TABLE_FEATURES);
	ofpmp_reply->flags = htonl(OFPMP_REPLY_MORE_NO);

	/*table0 classifier*/
	ofpmp_reply->ofpmp_table_features[0].table_id = 0;
	memcpy(ofpmp_reply->ofpmp_table_features[0].name,"clasifier",9);
	ofpmp_reply->ofpmp_table_features[0].metadata_match = 0xffffffffffffffff;
	ofpmp_reply->ofpmp_table_features[0].metadata_write = 0xffffffffffffffff;
	ofpmp_reply->ofpmp_table_features[0].config = 0;
	ofpmp_reply->ofpmp_table_features[0].max_entries = htonl(0x000f4240);

	table_features_prop_oft = sizeof(struct ofp_multipart)+sizeof(struct ofp_table_features);
	
	struct ofp_table_feature_prop_header *table_0_instructions = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	
	/*table feature property 0*/
	table_0_instructions->type = htons(OFPTFPT_INSTRUCTIONS);
	table_0_instructions->length = htons(28);	
	table_0_instructions->instruction_ids[0].type = htons(OFPIT_GOTO_TABLE);
	table_0_instructions->instruction_ids[0].len = htons(4);
	table_0_instructions->instruction_ids[1].type = htons(OFPIT_WRITE_METADATA);
	table_0_instructions->instruction_ids[1].len = htons(4);
	table_0_instructions->instruction_ids[2].type = htons(OFPIT_WRITE_ACTIONS);
	table_0_instructions->instruction_ids[2].len = htons(4);
	table_0_instructions->instruction_ids[3].type = htons(OFPIT_APPLY_ACTIONS);
	table_0_instructions->instruction_ids[3].len = htons(4);
	table_0_instructions->instruction_ids[4].type = htons(OFPIT_CLEAR_ACTIONS);
	table_0_instructions->instruction_ids[4].len = htons(4);
	table_0_instructions->instruction_ids[5].type = htons(OFPIT_METER);
	table_0_instructions->instruction_ids[5].len = htons(4);

	/*table feature property 1*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_instruction)*6+4;
	struct ofp_table_feature_prop_header *table_1_next_table_ids = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];	
	table_1_next_table_ids->type = htons(OFPTFPT_NEXT_TABLES);
	table_1_next_table_ids->length = htons(4);		
	//table_1_next_table_ids->next_table_ids[0].next_table_id = 1;
	
	/*table feature property 2*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_next_table)+4;
	struct ofp_table_feature_prop_header *table_2_write_actions = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_2_write_actions->type = htons(OFPTFPT_WRITE_ACTIONS);
	table_2_write_actions->length = htons(44);
	table_2_write_actions->action_ids[0].type = htons(OFPAT_SET_MPLS_TTL);
	table_2_write_actions->action_ids[0].length = htons(4);
	table_2_write_actions->action_ids[1].type = htons(OFPAT_PUSH_VLAN);
	table_2_write_actions->action_ids[1].length = htons(4);	
	table_2_write_actions->action_ids[2].type = htons(OFPAT_POP_VLAN);
	table_2_write_actions->action_ids[2].length = htons(4);
	table_2_write_actions->action_ids[3].type = htons(OFPAT_PUSH_MPLS);
	table_2_write_actions->action_ids[3].length = htons(4);	
	table_2_write_actions->action_ids[4].type = htons(OFPAT_POP_MPLS);
	table_2_write_actions->action_ids[4].length = htons(4);
	table_2_write_actions->action_ids[5].type = htons(OFPAT_SET_QUEUE);
	table_2_write_actions->action_ids[5].length = htons(4);	
	table_2_write_actions->action_ids[6].type = htons(OFPAT_GROUP);
	table_2_write_actions->action_ids[6].length = htons(4);
	table_2_write_actions->action_ids[7].type = htons(OFPAT_SET_NW_TTL);
	table_2_write_actions->action_ids[7].length = htons(4);	
	table_2_write_actions->action_ids[8].type = htons(OFPAT_DEC_NW_TTL);
	table_2_write_actions->action_ids[8].length = htons(4);
	table_2_write_actions->action_ids[9].type = htons(OFPAT_SET_FIELD);
	table_2_write_actions->action_ids[9].length = htons(4);	
	
	/*table feature property 3*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_action)*10+4;
	struct ofp_table_feature_prop_header *table_3_write_setfield = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_3_write_setfield->type = htons(OFPTFPT_WRITE_SETFIELD);
	table_3_write_setfield->length = htons(12);
	table_3_write_setfield->oxm_ids[0].classname = htons(OFPXMC_OPENFLOW_BASIC);
	table_3_write_setfield->oxm_ids[0].filed = 0x26;
	table_3_write_setfield->oxm_ids[0].length = 8;
	table_3_write_setfield->oxm_ids[1].classname = htons(OFPXMC_NXM_1);
	table_3_write_setfield->oxm_ids[1].filed = 0x1F;
	table_3_write_setfield->oxm_ids[1].length = 4;
	
	
	/*table feature property 4*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_oxm)*2+4;
	struct ofp_table_feature_prop_header *table_4_apply_actions = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_4_apply_actions->type = htons(OFPTFPT_APPLY_ACTIONS);
	table_4_apply_actions->length = htons(44);
	table_4_apply_actions->action_ids[0].type = htons(OFPAT_SET_MPLS_TTL);
	table_4_apply_actions->action_ids[0].length = htons(4);
	table_4_apply_actions->action_ids[1].type = htons(OFPAT_PUSH_VLAN);
	table_4_apply_actions->action_ids[1].length = htons(4);	
	table_4_apply_actions->action_ids[2].type = htons(OFPAT_POP_VLAN);
	table_4_apply_actions->action_ids[2].length = htons(4);
	table_4_apply_actions->action_ids[3].type = htons(OFPAT_PUSH_MPLS);
	table_4_apply_actions->action_ids[3].length = htons(4);	
	table_4_apply_actions->action_ids[4].type = htons(OFPAT_POP_MPLS);
	table_4_apply_actions->action_ids[4].length = htons(4);
	table_4_apply_actions->action_ids[5].type = htons(OFPAT_SET_QUEUE);
	table_4_apply_actions->action_ids[5].length = htons(4);	
	table_4_apply_actions->action_ids[6].type = htons(OFPAT_GROUP);
	table_4_apply_actions->action_ids[6].length = htons(4);
	table_4_apply_actions->action_ids[7].type = htons(OFPAT_SET_NW_TTL);
	table_4_apply_actions->action_ids[7].length = htons(4);	
	table_4_apply_actions->action_ids[8].type = htons(OFPAT_DEC_NW_TTL);
	table_4_apply_actions->action_ids[8].length = htons(4);
	table_4_apply_actions->action_ids[9].type = htons(OFPAT_SET_FIELD);
	table_4_apply_actions->action_ids[9].length = htons(4);	

	
	/*table feature property 5*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_action)*10+4;
	struct ofp_table_feature_prop_header *table_5_apply_setfield = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_5_apply_setfield->type = htons(OFPTFPT_APPLY_SETFIELD);
	table_5_apply_setfield->length = htons(12);
	table_5_apply_setfield->oxm_ids[0].classname = htons(OFPXMC_OPENFLOW_BASIC);
	table_5_apply_setfield->oxm_ids[0].filed = 0x26;
	table_5_apply_setfield->oxm_ids[0].length = 8;
	table_5_apply_setfield->oxm_ids[1].classname = htons(OFPXMC_NXM_1);
	table_5_apply_setfield->oxm_ids[1].filed = 0x1F;
	table_5_apply_setfield->oxm_ids[1].length = 4;
	
	/*table feature property 6*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_oxm)*2+4;
	struct ofp_table_feature_prop_header *table_6_instructions_miss = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_6_instructions_miss->type = htons(OFPTFPT_INSTRUCTIONS_MISS);
	table_6_instructions_miss->length = htons(28);
	table_6_instructions_miss->instruction_ids[0].type = htons(OFPIT_GOTO_TABLE);
	table_6_instructions_miss->instruction_ids[0].len = htons(4);
	table_6_instructions_miss->instruction_ids[1].type = htons(OFPIT_WRITE_METADATA);
	table_6_instructions_miss->instruction_ids[1].len = htons(4);
	table_6_instructions_miss->instruction_ids[2].type = htons(OFPIT_WRITE_ACTIONS);
	table_6_instructions_miss->instruction_ids[2].len = htons(4);
	table_6_instructions_miss->instruction_ids[3].type = htons(OFPIT_APPLY_ACTIONS);
	table_6_instructions_miss->instruction_ids[3].len = htons(4);
	table_6_instructions_miss->instruction_ids[4].type = htons(OFPIT_CLEAR_ACTIONS);
	table_6_instructions_miss->instruction_ids[4].len = htons(4);
	table_6_instructions_miss->instruction_ids[5].type = htons(OFPIT_METER);
	table_6_instructions_miss->instruction_ids[5].len = htons(4);
	
	/*table feature property 7*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_instruction)*6+4;
	struct ofp_table_feature_prop_header *table_7_next_tables_miss = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_7_next_tables_miss->type = htons(OFPTFPT_NEXT_TABLES_MISS);
	table_7_next_tables_miss->length = htons(5);
	table_7_next_tables_miss->next_table_ids[0].next_table_id = 1;
	
	/*table feature property 8*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_next_table);
	struct ofp_table_feature_prop_header *table_8_write_actions_miss = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_8_write_actions_miss->type = htons(OFPTFPT_WRITE_ACTIONS_MISS);
	table_8_write_actions_miss->length = htons(52);
	table_8_write_actions_miss->action_ids[0].type = htons(OFPAT_OUTPUT);
	table_8_write_actions_miss->action_ids[0].length = htons(4);
	table_8_write_actions_miss->action_ids[1].type = htons(OFPAT_SET_MPLS_TTL);
	table_8_write_actions_miss->action_ids[1].length = htons(4);
	table_8_write_actions_miss->action_ids[2].type = htons(OFPAT_DEC_MPLS_TTL);
	table_8_write_actions_miss->action_ids[2].length = htons(4);
	table_8_write_actions_miss->action_ids[3].type = htons(OFPAT_PUSH_VLAN);
	table_8_write_actions_miss->action_ids[3].length = htons(4);
	table_8_write_actions_miss->action_ids[4].type = htons(OFPAT_POP_VLAN);
	table_8_write_actions_miss->action_ids[4].length = htons(4);
	table_8_write_actions_miss->action_ids[5].type = htons(OFPAT_PUSH_MPLS);
	table_8_write_actions_miss->action_ids[5].length = htons(4);
	table_8_write_actions_miss->action_ids[6].type = htons(OFPAT_POP_MPLS);
	table_8_write_actions_miss->action_ids[6].length = htons(4);
	table_8_write_actions_miss->action_ids[7].type = htons(OFPAT_SET_QUEUE);
	table_8_write_actions_miss->action_ids[7].length = htons(4);
	table_8_write_actions_miss->action_ids[8].type = htons(OFPAT_GROUP);
	table_8_write_actions_miss->action_ids[8].length = htons(4);
	table_8_write_actions_miss->action_ids[9].type = htons(OFPAT_SET_NW_TTL);
	table_8_write_actions_miss->action_ids[9].length = htons(4);
	table_8_write_actions_miss->action_ids[10].type = htons(OFPAT_DEC_NW_TTL);
	table_8_write_actions_miss->action_ids[10].length = htons(4);
	table_8_write_actions_miss->action_ids[11].type = htons(OFPAT_SET_FIELD);
	table_8_write_actions_miss->action_ids[11].length = htons(4);
	/*table feature property 9*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_action)*12+4;
	struct ofp_table_feature_prop_header *table_9_write_setfield_miss = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_9_write_setfield_miss->type = htons(OFPTFPT_WRITE_SETFIELD_MISS);
	table_9_write_setfield_miss->length = htons(12);
	table_9_write_setfield_miss->oxm_ids[0].classname = htons(OFPXMC_OPENFLOW_BASIC);
	table_9_write_setfield_miss->oxm_ids[0].filed = 0x26;
	table_9_write_setfield_miss->oxm_ids[0].length = 8;
	table_9_write_setfield_miss->oxm_ids[1].classname = htons(OFPXMC_NXM_1);
	table_9_write_setfield_miss->oxm_ids[1].filed = 0x1F;
	table_9_write_setfield_miss->oxm_ids[1].length = 4;
	
	/*table feature property 10*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_oxm)*2+4;
	struct ofp_table_feature_prop_header *table_10_apply_actions_miss = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_10_apply_actions_miss->type = htons(OFPTFPT_APPLY_ACTIONS_MISS);
	table_10_apply_actions_miss->length = htons(52);
	table_10_apply_actions_miss->action_ids[0].type = htons(OFPAT_OUTPUT);
	table_10_apply_actions_miss->action_ids[0].length = htons(4);
	table_10_apply_actions_miss->action_ids[1].type = htons(OFPAT_SET_MPLS_TTL);
	table_10_apply_actions_miss->action_ids[1].length = htons(4);
	table_10_apply_actions_miss->action_ids[2].type = htons(OFPAT_DEC_MPLS_TTL);
	table_10_apply_actions_miss->action_ids[2].length = htons(4);
	table_10_apply_actions_miss->action_ids[3].type = htons(OFPAT_PUSH_VLAN);
	table_10_apply_actions_miss->action_ids[3].length = htons(4);
	table_10_apply_actions_miss->action_ids[4].type = htons(OFPAT_POP_VLAN);
	table_10_apply_actions_miss->action_ids[4].length = htons(4);
	table_10_apply_actions_miss->action_ids[5].type = htons(OFPAT_PUSH_MPLS);
	table_10_apply_actions_miss->action_ids[5].length = htons(4);
	table_10_apply_actions_miss->action_ids[6].type = htons(OFPAT_POP_MPLS);
	table_10_apply_actions_miss->action_ids[6].length = htons(4);
	table_10_apply_actions_miss->action_ids[7].type = htons(OFPAT_SET_QUEUE);
	table_10_apply_actions_miss->action_ids[7].length = htons(4);
	table_10_apply_actions_miss->action_ids[8].type = htons(OFPAT_GROUP);
	table_10_apply_actions_miss->action_ids[8].length = htons(4);
	table_10_apply_actions_miss->action_ids[9].type = htons(OFPAT_SET_NW_TTL);
	table_10_apply_actions_miss->action_ids[9].length = htons(4);
	table_10_apply_actions_miss->action_ids[10].type = htons(OFPAT_DEC_NW_TTL);
	table_10_apply_actions_miss->action_ids[10].length = htons(4);
	table_10_apply_actions_miss->action_ids[11].type = htons(OFPAT_SET_FIELD);
	table_10_apply_actions_miss->action_ids[11].length = htons(4);
	
	/*table feature property 11*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_action)*12+4;
	struct ofp_table_feature_prop_header *table_11_apply_setfield_miss = 
		(struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_11_apply_setfield_miss->type = htons(OFPTFPT_APPLY_SETFIELD_MISS);
	table_11_apply_setfield_miss->length = htons(12);
	table_11_apply_setfield_miss->oxm_ids[0].classname = htons(OFPXMC_OPENFLOW_BASIC);
	table_11_apply_setfield_miss->oxm_ids[0].filed = 0x26;
	table_11_apply_setfield_miss->oxm_ids[0].length = 8;
	table_11_apply_setfield_miss->oxm_ids[1].classname = htons(OFPXMC_NXM_1);
	table_11_apply_setfield_miss->oxm_ids[1].filed = 0x1F;
	table_11_apply_setfield_miss->oxm_ids[1].length = 4;
	
	/*table feature property 12*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)
		+sizeof(struct ofp_oxm)*2+4;
	struct ofp_table_feature_prop_header *table_12_match = (struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_12_match->type = htons(OFPTFPT_MATCH);
	table_12_match->length = htons(12);
	table_12_match->oxm_ids[0].classname = htons(OFPXMC_OPENFLOW_BASIC);
	table_12_match->oxm_ids[0].filed = 0x26;
	table_12_match->oxm_ids[0].length = 8;
	table_12_match->oxm_ids[1].classname = htons(OFPXMC_NXM_1);
	table_12_match->oxm_ids[1].filed = 0x1F;
	table_12_match->oxm_ids[1].length = 4;

	/*table feature property 13*/
	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)	+sizeof(struct ofp_oxm)*2+4;
	struct ofp_table_feature_prop_header *table_13_wildcards = (struct ofp_table_feature_prop_header *)&ofpbuf_reply->data[table_features_prop_oft];
	table_13_wildcards->type = htons(OFPTFPT_WILDCARDS);
	table_13_wildcards->length = htons(12);
	table_13_wildcards->oxm_ids[0].classname = htons(OFPXMC_OPENFLOW_BASIC);
	table_13_wildcards->oxm_ids[0].filed = 0x26;
	table_13_wildcards->oxm_ids[0].length = 8;
	table_13_wildcards->oxm_ids[1].classname = htons(OFPXMC_NXM_1);
	table_13_wildcards->oxm_ids[1].filed = 0x1F;
	table_13_wildcards->oxm_ids[1].length = 4;

	table_features_prop_oft += sizeof(struct ofp_table_feature_prop_header)	+ sizeof(struct ofp_oxm)*2+4;
	
	ofpbuf_reply->header.length = htons(sizeof(struct ofp_header)+table_features_prop_oft);
	//ofpmp_reply->ofpmp_table_features[0].length = 
		//htons(table_features_prop_oft-sizeof(struct ofp_table_features)-sizeof(struct ofp_multipart));
	ofpmp_reply->ofpmp_table_features[0].length = 
		htons(table_features_prop_oft-sizeof(struct ofp_multipart));
	
	send_openflow_message(ofpbuf_reply,sizeof(struct ofp_header)+table_features_prop_oft);
	
    return 0;
}

#endif



static enum ofperr
handle_ofpmp_port_desc(struct ofp_buffer *ofpbuf)
{
	int i = 0;
	int reply_len = sizeof(struct ofp_header)+sizeof(struct ofp_multipart)+ 
		sizeof(struct ofp_port)*nmps.cnt;
	struct ofp_buffer *ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_MULTIPART_REPLY,
		ofpbuf->header.xid,reply_len);
	struct ofp_multipart *ofpmp_reply = (struct ofp_multipart *)ofpbuf_reply->data;

	SHOW_FUN(0);
	ofpmp_reply->type = htons(OFPMP_PORT_DESC);
	ofpmp_reply->flags = htonl(OFPMP_REPLY_MORE_NO);	
	for(i=0;i<nmps.cnt;i++)
	{
		ofpmp_reply->ofpmp_port_desc[i] = nmps.ports[i].state;
	}	
	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);
	
    return 0;
}


static enum ofperr
handle_multipart_request(struct ofp_buffer *ofpbuf)
{	
	struct ofp_multipart *request = (struct ofp_multipart *)ofpbuf->data;
	int ofpmp_type = ntohs(request->type);

	SHOW_FUN(0);
	LOG_DBG("ofpbuf->header.type=%d{ofpmp_type=%d}\n",ofpbuf->header.type,ofpmp_type);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	switch(ofpmp_type)
	{
		case OFPMP_DESC:
			return handle_ofpmp_desc(ofpbuf);
			
		case OFPMP_FLOW:
			return handle_ofpmp_flow_stats(ofpbuf);
			
		case OFPMP_AGGREGATE:
			return handle_ofpmp_aggregate(ofpbuf);
		

		case OFPMP_PORT_STATS:
			return handle_ofpmp_port_stats(ofpbuf);
#if 0


		case OFPMP_TABLE:
			return handle_ofpmp_table(ofpbuf);
		case OFPMP_QUEUE:
			return handle_ofpmp_queue(ofpbuf);

		case OFPMP_GROUP:
			return handle_ofpmp_group(ofpbuf);

		case OFPMP_GROUP_DESC:
			return handle_ofpmp_group_desc(ofpbuf);

		case OFPMP_GROUP_FEATURES:
			return handle_ofpmp_group_features(ofpbuf);
			
		case OFPMP_METER:
			return handle_ofpmp_meter(ofpbuf);

		case OFPMP_METER_CONFIG:
			return handle_ofpmp_meter_config(ofpbuf);

		case OFPMP_METER_FEATURES:
			return handle_ofpmp_meter_features(ofpbuf);
			
		case OFPMP_EXPERIMENTER:
			return handle_ofpmp_experimenter(ofpbuf);
#endif			
		case OFPMP_TABLE_FEATURES:
			return handle_ofpmp_table_features(ofpbuf);
			
		case OFPMP_PORT_DESC:
			return handle_ofpmp_port_desc(ofpbuf);

		
			
		default:
			LOG_DBG("handle_multipart_request DEFAULT\n");
	}
	SHOW_FUN(1);
	return 0;
}

static enum ofperr
handle_queue_get_config_request(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	//ofpbuf->header.type = OFPT_BARRIER_REPLY;
	//send_openflow_message(ofpbuf,ofpbuf->header.length);
	SHOW_FUN(1);
	return 0;
}

static enum ofperr
handle_barrier_request(struct ofp_buffer *ofpbuf)
{
	int reply_len = sizeof(struct ofp_header);	
	struct ofp_buffer *ofpbuf_reply = (struct ofp_buffer *)build_reply_ofpbuf(OFPT_BARRIER_REPLY,
		ofpbuf->header.xid,reply_len);
	
	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);
	
	return 0;
}

static enum ofperr
handle_role_request(struct ofp_buffer *ofpbuf)
{	
	int reply_len = sizeof(struct ofp_header)+sizeof(struct ofp_role);	
	struct ofp_buffer *ofpbuf_reply = 
		(struct ofp_buffer *)build_reply_ofpbuf(OFPT_ROLE_REPLY,ofpbuf->header.xid,reply_len);
	
	SHOW_FUN(0);
	memcpy(ofpbuf_reply->data,ofpbuf->data,sizeof(struct ofp_role));	
	ofpbuf_reply->header.type = OFPT_ROLE_REPLY;	
	send_openflow_message(ofpbuf_reply,reply_len);
	SHOW_FUN(1);
	
	return 0;
}

static enum ofperr
handle_get_async_request(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	//ofpbuf->header.type = OFPT_BARRIER_REPLY;
	//send_openflow_message(ofpbuf,ofpbuf->header.length);
	SHOW_FUN(1);
	return 0;
}

static enum ofperr
handle_set_async(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	//ofpbuf->header.type = OFPT_BARRIER_REPLY;
	//send_openflow_message(ofpbuf,ofpbuf->header.length);
	SHOW_FUN(1);
	return 0;
}

static enum ofperr
handle_meter_mod(struct ofp_buffer *ofpbuf)
{
	SHOW_FUN(0);
	pkt_print((u8 *)ofpbuf,htons(ofpbuf->header.length));
	//ofpbuf->header.type = OFPT_BARRIER_REPLY;
	//send_openflow_message(ofpbuf,ofpbuf->header.length);
	SHOW_FUN(1);
	return 0;
}


static enum ofperr
handle_openflow(struct ofp_buffer *ofpbuf,int len)
{	
	int oftype = ofpbuf->header.type;	
	
	SHOW_FUN(0);	
	LOG_DBG("ofpbuf->header.type=%d\n",ofpbuf->header.type);	
	switch(oftype)
	{
		case OFPT_HELLO:
			return handle_hello(ofpbuf);
       
		case OFPT_ERROR:
			return handle_error(ofpbuf);
			
		case OFPT_ECHO_REQUEST:
			return handle_echo_request(ofpbuf);

		case OFPT_EXPERIMENTER:
			return handle_experimenter(ofpbuf);

		case OFPT_FEATURES_REQUEST:
			return handle_features_request(ofpbuf);

		case OFPT_GET_CONFIG_REQUEST:
			return handle_get_config_request(ofpbuf);
		
		case OFPT_SET_CONFIG:
			return handle_set_config(ofpbuf,len);

		case OFPT_PACKET_OUT:
			return handle_packet_out(ofpbuf);

		case OFPT_FLOW_MOD:
			return handle_flow_mod(ofpbuf);

		case OFPT_GROUP_MOD:
			return handle_group_mod(ofpbuf);
			
		case OFPT_PORT_MOD:
			return handle_port_mod(ofpbuf);
			
		case OFPT_TABLE_MOD:
			return handle_table_mod(ofpbuf);		

		case OFPT_MULTIPART_REQUEST:
			return handle_multipart_request(ofpbuf);

		case OFPT_QUEUE_GET_CONFIG_REQUEST:
			return handle_queue_get_config_request(ofpbuf);

		case OFPT_BARRIER_REQUEST:
			return handle_barrier_request(ofpbuf);		

		case OFPT_ROLE_REQUEST:
			return handle_role_request(ofpbuf);

		case OFPT_GET_ASYNC_REQUEST:
			return handle_get_async_request(ofpbuf);

		case OFPT_SET_ASYNC:
			return handle_set_async(ofpbuf);

		case OFPT_METER_MOD:
			return handle_meter_mod(ofpbuf);

		
		case OFPT_ECHO_REPLY:			
		case OFPT_PACKET_IN:
		case OFPT_FLOW_REMOVED:
		case OFPT_PORT_STATUS:
		case OFPT_FEATURES_REPLY:
		case OFPT_GET_CONFIG_REPLY:			
		case OFPT_MULTIPART_REPLY:
		case OFPT_BARRIER_REPLY:
		case OFPT_QUEUE_GET_CONFIG_REPLY:			
		case OFPT_ROLE_REPLY:
		case OFPT_GET_ASYNC_REPLY:		
		
	
		default:
			LOG_DBG("not handle the message!\n");
		

		
	}
	SHOW_FUN(1);
	return 0;
}


void *recv_openflow_message(void *argv)
{
	int recv_len;
	struct ofp_buffer *ofpbuf = (struct ofp_buffer *)malloc(MAXLINE);
	int ofp_head_len = sizeof(struct ofp_header);
	char thread_name[16] = {0};
	u8 tid = 0;

	SHOW_FUN(0);
	sprintf(thread_name,"%X_nms_%s",tid,"of13");
	thread_name[15] = '\0';
	prctl(PR_SET_NAME,thread_name);	
	send_hello_message();/*成功启动接收线程后再向控制器发送HELLO!*/
	
	while(1){
		if((recv_len=read(ofpfd,(u8*)ofpbuf,ofp_head_len))<1){
			continue;
		}
		if(htons(ofpbuf->header.length)>ofp_head_len){
			recv_len += read(ofpfd,(u8*)ofpbuf + ofp_head_len,htons(ofpbuf->header.length) - ofp_head_len);
		}
		print_idx_nms[tid] = 0;
		LOG_DBG("################################################\n\n");
		LOG_DBG("ofp:%p,recv_len=%d\n",ofpbuf,recv_len);
		if(recv_len >= MAXLINE)
			LOG_ERR("Hei Hei,recv_len:%d >= %d\n",recv_len,MAXLINE);
		handle_openflow(ofpbuf,recv_len);		
	}
	free(ofpbuf);
	SHOW_FUN(1);
	return 0;
}

pthread_t openflow_listener(char *controller_ip)
{
	pthread_t tid;
	
	SHOW_FUN(0);
	open_openflow_connect(controller_ip);
			
	if(pthread_create(&tid, NULL, recv_openflow_message, NULL))
	{
		LOG_ERR("Create recv_openflow_message thread error!\n");
	}
	else
	{
		LOG_DBG("Create handle_openflow_message thread OK!\n");
	}
	SHOW_FUN(1);
	return tid;
}

void handle_pcap_packet(int inport,struct eth_header *eth,int len)
{
	u16 frame_type = ntohs(eth->frame);
	
	SHOW_FUN(0);
	LOG_DBG("%s handle_pcap_packet\n\n",nmps.ports[inport].name);
	switch(frame_type)
	{
		case ETH_P_LLDP:
			send_packet_in_message_lldp(inport,(u8 *)eth,len);
			break;
		default:
			send_packet_in_message_normal(inport,(u8 *)eth,len);
			break;
	}
	SHOW_FUN(1);
}

void *pcap_packet(void *argv)
{
	int *idx = (int *)argv;
	struct pcap_pkthdr hdr;
	const u_char *pkt;
	static u32 ts = 0;
	char thread_name[16] = {0};
	struct meter_buffer *meter;	
	pcap_t *pcap_handle = nmps.ports[*idx].pcap;
	u8 tid = *idx + 1;
	
	print_idx_nms[tid] = 0;
	SHOW_FUN(0);
	sprintf(thread_name,"%X_nms_%s",tid,nmps.ports[*idx].name);
	thread_name[15] = '\0';
	prctl(PR_SET_NAME,thread_name);
	while(1)
	{
		pkt = pcap_next(pcap_handle, &hdr);
		if(pkt)
		{	
			print_idx_nms[tid] = 0;
			if(hdr.caplen>1514)
			{
				LOG_DBG("PCAP DROP %s len > 1514!",nmps.ports[*idx].name);
				continue;
			}
#ifdef NMS_METER
			LOG_DBG("METER %s ->packet:%p,type:%04X,len:%d\n",nmps.ports[*idx].name,pkt,ntohs(((struct eth_header *)pkt)->frame),hdr.caplen);	
			ts += 0x23;
			meter = (struct meter_buffer *)(pkt + 14);
			meter->in_port = 0;
			meter->ts =ts;					
			send_packet_in_message_meter(*idx,(u8 *)meter,sizeof(*meter));//OFPR_ACTION->controller
#else			
			LOG_DBG("HANDLE_PCAP %s ->packet:%p,type:%04X,len:%d\n",nmps.ports[*idx].name,pkt,ntohs(((struct eth_header *)pkt)->frame),hdr.caplen);	
			handle_pcap_packet(*idx,(struct eth_header *)pkt,hdr.caplen);
#endif
		}	
	}
	free(argv);
	SHOW_FUN(1);
}

void start_pcap(int port)
{
	pthread_t tid;
	int *idx = malloc(sizeof(int *));	
	
	SHOW_FUN(0);
	*idx = port;	
	if(pthread_create(&tid, NULL, pcap_packet, (void *)idx)){
		LOG_ERR("Create %s pcap_packet thread error!\n",nmps.ports[port].name);		
	}
	else
	{
		LOG_DBG("Create [ %s ] pcap_packet thread OK!\n",nmps.ports[port].name);			
	}
	SHOW_FUN(1);
}

void PORT_REG_WR(int port,u64 regaddr,u64 regvalue)
{	
	return fast_reg_wr(FAST_PORT_BASE|(FAST_PORT_OFT*port)|(regaddr<<3),regvalue);
}
u64 PORT_REG(int port,u64 regaddr)
{	
	return fast_reg_rd(FAST_PORT_BASE|(FAST_PORT_OFT*port)|(regaddr<<3));
}

void *update_port_info(void *argv)
{
	u64 pcs_status = 0,neg_status = 0;
	int port = 0,i = 0;
	u16 duplex = 0,speed = 0;
	u32 curr = 0;
	while(1)
	{
		for(i=0;i<nmps.cnt;i++)
		{
			PORT_REG_WR(port,0xf,port);
			duplex = 0;
			speed = 0;
			curr = 0;
			port = nmps.ports[i].phyport;/*取逻辑端口的物理端口号*/
			pcs_status = PORT_REG(port,FAST_PORT_PCS_STATUS);
			neg_status = PORT_REG(port,FAST_PORT_NEG_STATUS);

			nmps.ports[i].state.config = htonl(0);
			nmps.ports[i].state.state = htonl((neg_status&FAST_PORT_NEG_STATUS_UP)==FAST_PORT_NEG_STATUS_UP?1:0);

			if((neg_status&FAST_PORT_NEG_STATUS_FULL)==0)
				duplex = 0;
			else
				duplex = 1;
			speed = (neg_status&FAST_PORT_NEG_STATUS_100M)==FAST_PORT_NEG_STATUS_100M?100:((neg_status&FAST_PORT_NEG_STATUS_1G)==FAST_PORT_NEG_STATUS_1G?1000:10);
			if(speed == 10)
				curr = 1<<(duplex==1?1:0);
			else if(speed == 100)
				curr = 1<<(duplex==1?3:0);
			else if(speed == 1000)
				curr = 1<<(duplex==1?5:0);
			if(port < 8)/*千兆电口*/
			{
				curr |= 1<<11;/*COPPER*/
			}
			else/*万兆光口*/
			{
				curr = 1<<6;/*10G*/
				curr |= 1<<12;/*FIBER*/
			}
			if((pcs_status&FAST_PORT_PCS_STATUS_AUTONEG_OK)>0)
			{
				curr |= 1<<13;
			}			
			nmps.ports[i].state.curr = htonl(curr);//0x2820

			nmps.ports[i].state.advertised = htonl(0x282f);
			nmps.ports[i].state.supported = htonl(0x282f);
			nmps.ports[i].state.peer = htonl(0);

			nmps.ports[i].state.curr_speed = htonl(0x1000000);
			nmps.ports[i].state.max_speed= htonl(0x1000000);

			nmps.ports[i].stats.rx_bytes 		= htonll((PORT_REG(port,FAST_COUNTS_RECV_D1_OK)<<32)|PORT_REG(port,FAST_COUNTS_RECV_D0_OK));
			nmps.ports[i].stats.rx_packets		= htonll(PORT_REG(port,FAST_COUNTS_RECV_OK));
			nmps.ports[i].stats.rx_errors 		= htonll(PORT_REG(port,FAST_COUNTS_RCVFERR));
			nmps.ports[i].stats.rx_dropped 		= htonll(0);			
			nmps.ports[i].stats.rx_frame_err 	= htonll(0);

			nmps.ports[i].stats.tx_bytes 		= htonll((PORT_REG(port,FAST_COUNTS_SEND_D1_OK)<<32)|PORT_REG(port,FAST_COUNTS_SEND_D0_OK));
			nmps.ports[i].stats.tx_packets 		= htonll(PORT_REG(port,FAST_COUNTS_SEND_OK));
			nmps.ports[i].stats.tx_errors 		= htonll(PORT_REG(port,FAST_COUNTS_SNDFERR));
			nmps.ports[i].stats.tx_dropped 		= htonll(0);			
			nmps.ports[i].stats.collisions 		= htonll(0);	
		}
		sleep(5);/*每5秒钟更新一次*/
	}
}

char *get_value_by_shell(char *cmd)
{
	char value[1024] = {0};
	int len = 0;
	FILE *stream = popen(cmd,"r");

	len = fread(value,sizeof(char),sizeof(value),stream);
	pclose (stream);
	return NULL;
}


void get_dev_mac(char *if_name,char *hw_addr)
{
	int sock;
	struct ifreq ifr;

	if((sock = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		LOG_ERR("Read %s MAC ERROR!\n",if_name);
	}

	strcpy(ifr.ifr_name,if_name);
	if(ioctl(sock,SIOCGIFHWADDR,&ifr)<0)
	{
		LOG_ERR("Read %s MAC ioctl ERROE!\n",if_name);
	}
	memcpy(nmps.ports[nmps.cnt].port_mac,ifr.ifr_hwaddr.sa_data,6);
	*((uint64 *)&nmps.ports[nmps.cnt].state.hw_addr) = htonll((*((uint64 *)&ifr.ifr_hwaddr.sa_data)&0xFFFFFFFFFFFF)<<16);	
}

static inline int IN6_IS_MY_ADDR6(struct in6_addr *addr6)
{
	return htons(*((u16 *)addr6)) == 0x2001;/*我们地址的标识,863项目中使用到*/
}

void get_dev_ip6(char *if_name,struct in6_addr *addr6)
{
	struct ifaddrs *ifa,*p;
	char ip6[128] = {0};

	if(getifaddrs(&ifa))
	{
		LOG_ERR("getifaddrs");
	}

	for(p = ifa; p!=NULL;p=p->ifa_next)
	{
		if(p->ifa_addr->sa_family == AF_INET6 && !strncmp(if_name,p->ifa_name,strlen(if_name)))
		{
			*addr6 = ((struct sockaddr_in6 *)(p->ifa_addr))->sin6_addr;
			if(IN6_IS_MY_ADDR6(addr6))
			{
				inet_ntop(AF_INET6,(void *)addr6,ip6,sizeof(ip6));
				LOG_DBG("%s IPv6 Addr:%s\n",if_name,ip6);
				return;
			}		
		}
	}	
	memset(addr6,0,sizeof(struct in6_addr));
}

int get_phy_port(char *port_name)
{
	printf("port_name:%s,port:%d\n",port_name,(port_name[strlen(port_name)-1] - 48));
	return (int)(port_name[strlen(port_name)-1] - 48);
}

void init_port(void)
{
	char *port_name;
	char *ptr = NULL;
	char *tmp = port_list_info;
	char errbuf[255];
	struct timeval t;
	struct bpf_program bpf_filter;
	char bpf_filter_string[256] = " ";
	int idx = 0;
	struct in6_addr addr6;

	memset((char *)&nmps,0,sizeof(nmps));
	while((port_name = strtok_r(tmp,",",&ptr)))
	{
		gettimeofday(&t,NULL);
		idx = nmps.cnt;
		nmps.ports[idx].logport = idx;
		nmps.ports[idx].phyport = get_phy_port(port_name);
		/*---------PORT STATE----------------*/
		memcpy(nmps.ports[idx].name,port_name,strlen(port_name));
		memcpy(nmps.ports[idx].state.name,port_name,strlen(port_name));
		nmps.ports[idx].state.port_no = htonl(idx);
		get_dev_mac(port_name,nmps.ports[idx].state.hw_addr);
		get_dev_ip6(port_name,&nmps.ports[idx].port_ip6);
		nmps.ports[idx].state.config = htonl(0);
		nmps.ports[idx].state.state = htonl(0);
		nmps.ports[idx].state.curr = htonl(0x2820);
		nmps.ports[idx].state.advertised = htonl(0x282f);
		nmps.ports[idx].state.supported = htonl(0x282f);
		nmps.ports[idx].state.peer = htonl(0);
		nmps.ports[idx].state.curr_speed = htonl(0x1000000);
		nmps.ports[idx].state.max_speed= htonl(0x1000000);
		nmps.ports[idx].stats.port_no = htonl(idx);	
		/*---------INIT libpcap----------------*/
		nmps.ports[idx].pcap = pcap_open_live(port_name, BUFSIZ, 1, 0, errbuf);
		if(nmps.ports[idx].pcap == NULL)
		{
			LOG_ERR("pcap_open_live[%s]: %s ERROR!\n",port_name,errbuf);
		}	
		if(-1 == pcap_compile(nmps.ports[idx].pcap, &bpf_filter, bpf_filter_string, 0, PCAP_NETMASK_UNKNOWN))
		{			
			LOG_ERR("pcap_compile[%s]: %s ERROR!\n",port_name,errbuf);
		}
		if(-1 == pcap_setdirection(nmps.ports[idx].pcap,PCAP_D_IN))
		{
			pcap_perror(nmps.ports[idx].pcap,"pcap_setdirection");			
		}
		if(-1 == pcap_setfilter(nmps.ports[idx].pcap, &bpf_filter))
		{
			LOG_ERR("pcap_setfilter[%s]: %s ERROR!\n",port_name,errbuf);
		}
		/*---------INIT libnet----------------*/
		nmps.ports[idx].net = libnet_init(LIBNET_LINK, port_name, errbuf);
		if(nmps.ports[idx].net == NULL)
		{
			LOG_ERR("libnet_init[%s]: %s ERROR!\n",port_name,errbuf);
		}		
		pthread_mutex_init(&nmps.ports[idx].port_send_mutex,NULL);
		nmps.cnt++;	
		tmp = NULL;		
	}		
	//LOG_ERR("OK!\n");
}

void usage(char *argv)
{
	printf("Usage:\n\t %s  -{4|6} controllerIP -i obx0,obx1,[nm1,nm2,..] [-v level] [-c fl|odl]\n",argv);
	printf("\t-4 IPv4地址\t-6 IPv6地址（必选参数）\n");
	printf("\t-i 添加SDN交换机的端口（OpenBox平台：obx0,obx1...;NetMagic08平台：nm1,nm2...）（必选参数）\n");
	printf("\t-v 日志打印参数，每个位表示一个进程（端口）（可选参数）\n");
	printf("\t-c 控制器类别fl:Floodlight,odl:OpenDayLight（可选参数）\n");
	exit(0);
}

void parse_options(int argc,char *argv[])
{
	int i = 1,j = 0,ip_flag = 0;
	if(argc < 5)
	{
		usage(argv[0]);
	}
	for(;i < argc - 1;i++)
	{
		if((!strncmp("-4",argv[i],2)&&(ip_type=4)>0)||(!strncmp("-6",argv[i],2)&&(ip_type=6)>0))
		{
			if(ip_flag == 0)
			{
				memcpy(controller_ip,argv[i+1],strlen(argv[i+1]));			
				i++;
				j++;
				ip_flag = 1;
			}
			else
			{
				usage(argv[0]);
			}
		}		
		else if(ip_flag && !strncmp("-i",argv[i],2))
		{			
			memcpy(port_list_info,argv[i+1],strlen(argv[i+1]));			
			i++;
			j++;
		}		
		else if(!strncmp("-v",argv[i],2))
		{	
			sscanf(argv[i+1],"%X",&debug);		
			i++;			
		}		
		else if(!strncmp("-c",argv[i],2))
		{	
			if(!strncmp("fl",argv[i+1],2))
			{
				controller_port = 6653;
			}
			else if(!strncmp("odl",argv[i+1],3))
			{
				controller_port = 6633;/*6653同样可以*/
			}			
			i++;			
		}
	}
		if(j != 2)
	{
		usage(argv[0]);
	}
}

void start_port_pcap(void)
{
	int i = 0;
	for(;i<nmps.cnt;i++)
	{
		start_pcap(i);
		thread_idx++;
	}
}

void start_port_info_update(void)
{
	pthread_t tid;
		
	SHOW_FUN(0);	
	if(pthread_create(&tid, NULL, update_port_info, NULL))
	{
		LOG_ERR("Create update_port_info thread error!\n");		
	}
	else
	{
		LOG_DBG("Create update_port_info thread OK!\n");			
	}
	SHOW_FUN(1);
}

void ofp_exit(void)
{
	close_openflow_connect();
	exit(0);
}

void __ofp_exit(int argc)
{
	LOG_DBG("ofp_exit!\n");
	ofp_exit();
}

int ofp_init(int argc,char *argv[])
{
	pthread_t ofp_tid;
		
	debug = 0;
	parse_options(argc,argv);
	
	SHOW_FUN(0);
	gettimeofday(&start_tv,NULL);
	start_tv.tv_usec = 0;	
	init_port();//初始化端口信息表
	fast_init_hw(0,0);
	init_rule(ACTION_SET_MID<<28|FAST_DMID_PROTO_STACK);//规则模块初始化，默认规则是送CPU的内核协议栈
	ofp_tid = openflow_listener(controller_ip);
	//pthread_join(ofp_tid, NULL);
	
	start_port_pcap();//启动端口捕包线程	
	start_port_info_update();//启动端口计数与状态更新线程
	signal(SIGINT,__ofp_exit);//非法结束时，调用注销函数
	SHOW_FUN(1);	
}

