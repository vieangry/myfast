/***************************************************************************
 *            main_port_status.c
 *
 *  2017/03/05 20:58:44 星期日
 *  Copyright  2017  XuDongLai
 *  <XuDongLai0923@163.com>
 ****************************************************************************/
/*
 * main_port_status.c
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
#include "../../include/fast.h"

u64 PORT_REG(int port,u64 regaddr)
{		
	return fast_reg_rd(FAST_PORT_BASE|(FAST_PORT_OFT*port)|(regaddr<<3));
}
void show_head_info(void)
{
	printf("PORT      <MAC ADDR> <FRAME LEN> <FRAME SPACE> <BUF LEVEL>  <PCS MODE> <PCS STUTUS> <NEG STATUS> <LINK> <AUTONEG EN> <AUTONEG> <SPEED> <DUPLEX>\n");
}
void show_port_status(int port)
{
	u64 pcs_status = PORT_REG(port,FAST_PORT_PCS_STATUS),neg_status = PORT_REG(port,FAST_PORT_NEG_STATUS);
	printf("%d    0x%08lX%04lX        %4ld            %2ld          %2ld         0x%lX         0x%02lX   0x%08lX %6s          %3s       %3s    %4d     %4s\n",
	       port,
	       PORT_REG(port,FAST_PORT_MAC_0),
	       PORT_REG(port,FAST_PORT_MAC_1),
	       PORT_REG(port,FAST_PORT_FRAME_MAX_LEN),
	       PORT_REG(port,FAST_PORT_FRAME_SPACE),
	       PORT_REG(port,FAST_PORT_BUF_LEVEL),
	       PORT_REG(port,FAST_PORT_PCS_MODE),
	       pcs_status,
	       neg_status,
	       (neg_status&FAST_PORT_NEG_STATUS_UP)==FAST_PORT_NEG_STATUS_UP?"UP":"DOWN",
	       (pcs_status&FAST_PORT_PCS_STATUS_AUTONEG_EN)>0?"YES":"NO",
	       (pcs_status&FAST_PORT_PCS_STATUS_AUTONEG_OK)>0?"YES":"NO",
	       (neg_status&FAST_PORT_NEG_STATUS_100M)==FAST_PORT_NEG_STATUS_100M?100:((neg_status&FAST_PORT_NEG_STATUS_1G)==FAST_PORT_NEG_STATUS_1G?1000:10),
	       (neg_status&FAST_PORT_NEG_STATUS_FULL)==0?"half":"full");
}

int main(int argc,char *argv[])
{
	int i = 0,port = -1;
	
	fast_init_hw(0,0);
	printf("---------------------SHOW_PORT_STATUS---------------------\n");	
	show_head_info();
	if(argc == 1)/*显示所有端口的计算信息*/
	{
		for(i = 0;i< 10;i++)
		{
			show_port_status(i);
		}
	}
	else if((port = atoi(argv[1])) > -1 && port < 10)
	{
		show_port_status(port);
	}	
	return 0;
}