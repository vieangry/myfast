/***************************************************************************
 *            main_port_counts.c
 *
 *  2017/03/05 20:55:12 星期日
 *  Copyright  2017  XuDongLai
 *  <XuDongLai0923@163.com>
 ****************************************************************************/
/*
 * main_port_counts.c
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
	printf("PORT   <Send OK> <SNDSPKT> <SNDMPKT> <SNDBPKT> | <Recv OK> <RCVSPKT> <RCVMPKT> <RCVBPKT> | <CRC ERR> <ALIGNER> <SNDFERR> <RCVFERR>\n");
}
void show_port_counts(int port)
{
	printf("%d      %8lX  %8lX  %8lX  %8lX  | %8lX  %8lX  %8lX  %8lX  | %8lX  %8lX  %8lX  %8lX\n",
	       port,
	       PORT_REG(port,FAST_COUNTS_SEND_OK),
	       PORT_REG(port,FAST_COUNTS_SNDSPKT),
	       PORT_REG(port,FAST_COUNTS_SNDMPKT),
	       PORT_REG(port,FAST_COUNTS_SNDBPKT),
	       PORT_REG(port,FAST_COUNTS_RECV_OK),
	       PORT_REG(port,FAST_COUNTS_RCVSPKT),
	       PORT_REG(port,FAST_COUNTS_RCVMPKT),
	       PORT_REG(port,FAST_COUNTS_RCVBPKT),
	       PORT_REG(port,FAST_COUNTS_CRC_ERR),
	       PORT_REG(port,FAST_COUNTS_ALIGNER),
	       PORT_REG(port,FAST_COUNTS_SNDFERR),
	       PORT_REG(port,FAST_COUNTS_RCVFERR));
}

int main(int argc,char *argv[])
{
	int i = 0,port = -1;
	
	fast_init_hw(0,0);	
	printf("---------------------SHOW_PORT_COUNTS---------------------\n");	
	show_head_info();
	if(argc == 1)/*显示所有端口的计算信息*/
	{
		for(i = 0;i< 10;i++)
		{
			show_port_counts(i);
		}
	}
	else if((port=atoi(argv[1])) > -1 && port < 10)
	{
		show_port_counts(port);
	}	
	return 0;
}