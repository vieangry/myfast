/***************************************************************************
 *            main_reg_rw.c
 *
 *  2017/03/06 21:25:42 星期一
 *  Copyright  2017  XuDongLai
 *  <XuDongLai0923@163.com>
 ****************************************************************************/
/*
 * main_reg_rw.c
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
	printf("Usage:\n\t%s {rd|wr} regaddr [regvalue]\n",argv);
	exit(0);
}

int main(int argc,char *argv[])
{	
	u64 regaddr = 0,regvalue = 0;

	if(argc != 3 && argc != 4)Usage(argv[0]);
	
	if(fast_init_hw(0,0) != 0)
	{
		printf("Init FAST HW ERR!\n");
		exit(0);
	}

	if(argc == 3 && !strncmp("rd",argv[1],2))/*读寄存器*/
	{
		if(sscanf(argv[2],"%lX",&regaddr)==1)
		{
			printf("REG READ ->Addr:0x%lX = 0x%lX\n",regaddr,fast_reg_rd (regaddr));
		}		
	}
	else if(argc == 4 && !strncmp("wr",argv[1],2))/*写寄存器*/
	{
		if(sscanf(argv[2],"%lX",&regaddr)==1 && sscanf(argv[3],"%lX",&regvalue)==1)
		{
			u64 regvalue_back = 0;
			fast_reg_wr (regaddr,regvalue);
			regvalue_back = fast_reg_rd (regaddr);
			printf("REG WRITE->Addr:0x%lX = 0x%lX%s\n",regaddr,regvalue_back,regvalue==regvalue_back?"(OK)":"(ER)");
		}
	}	
	else
	{
		Usage(argv[0]);
	}
	return 0;
}