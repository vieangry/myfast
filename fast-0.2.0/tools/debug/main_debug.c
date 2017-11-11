/***************************************************************************
 *            main_debug.c
 *
 *  2017/03/06 22:20:27 星期一
 *  Copyright  2017  XuDongLai
 *  <XuDongLai0923@163.com>
 ****************************************************************************/
/*
 * main_debug.c
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

void Usage(char *argv)
{
	printf("Usage:\n\t%s port(x) regaddr\n",argv);
	exit(0);
}

u64 PORT_REG(int port,u64 regaddr)
{		
	return fast_reg_rd(FAST_PORT_BASE|(FAST_PORT_OFT*port)|(regaddr<<3));
}

void show_port_reg_info(int id,int port,int regaddr)
{
	printf("%2d     %2d   0x%X(0x%02X)     0x%08lX\n",id,port,FAST_PORT_BASE|(FAST_PORT_OFT*port)|(regaddr<<3),regaddr,PORT_REG(port,(u64)regaddr));
}

int main(int argc,char *argv[])
{
	int i = 0,port = -1;

	if(argc != 3)
	{
		Usage(argv[0]);
	}	
	fast_init_hw(0,0);	
	printf("---------------------DEBUG---------------------\n");
	printf("ID   PORT             REG          VALUE\n");
	if(!strncmp("x",argv[1],1))/*读所有端口对应寄存器*/
	{
		for(i = 0;i< 10;i++)
		{
			show_port_reg_info(i,i,atoi(argv[2]));
		}
	}
	else if((port = atoi(argv[1])) > -1 && port < 10)
	{
		show_port_reg_info(0,port,atoi(argv[2]));
	}
	return 0;
}