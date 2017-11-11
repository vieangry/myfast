/***************************************************************************
 *            main_rule.c
 *
 *  2017/03/05 22:13:29 星期日
 *  Copyright  2017  XuDongLai
 *  <XuDongLai0923@163.com>
 ****************************************************************************/
/*
 * main_rule.c
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
	printf("Usage:\n\t%s {hw|sw|cp}\n",argv);
	exit(0);
}
int main(int argc,char *argv[])
{
	if(argc != 2)
	{
		Usage(argv[0]);
	}
	fast_init_hw(0,0);
	if(!strncmp("hw",argv[1],2))/*打印输出硬件规则内容*/
	{
		printf("---------------------SHOW_HW_RULE---------------------\n");
		print_hw_rule();
	}
	else if(!strncmp("cp",argv[1],2))/*对比打印软硬件规则差异,看是否有同步*/
	{
		printf("---------------------SHOW_HW_CMP_SW_RULE---------------------\n");
		//print_hw_cmp_sw_rule();
	}
	else/*默认软件规则打印*/
	{
		print_sw_rule();//此处无法实现,因为软件规则只存储在用户的进程中,我们无法读到(除非缓存进内核)
	}
	return 0;
}