/***************************************************************************
 *            main_librule.c
 *
 *  2017/01/05 13:24:53 星期四
 *  Copyright  2017  XuDongLai
 *  <XuDongLai0923@163.com>
 ****************************************************************************/
/*
 * main_librule.c
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

#define RULE_LEN sizeof(struct fast_rule)   /*256B*/

#define RULE_VERSION "1.0.1"
struct rule_table
{
	u64 cnt;
	u64 pad;
	struct fast_rule rules[FAST_RULE_CNT];
};

struct reg_value{
		u32 v[1];
};


int rule_idx = 0;
struct rule_table table = {0,0,{0}};
struct fast_rule zero_rule = {0};


u16 n2rule16(u16 n)
{
	return htons(1) == 1 ? htole16(n):n;
}

u32 n2rule32(u32 n)
{
	return htons(1) == 1 ? htole32(n):n;
}

u64 n2rule64(u64 n)
{
	return htons(1) == 1 ? htole64(n):n;
}

void set_rule_mac64(char *mac,u64 value)
{
	if(htons(1) == 1)
	{
		int i = 0;
		u64 v = value;
		for(;i<6;i++)
		{
			mac[i] = v & 0xFF;
			v >>= 8;
		}			
	}
	else
	{
		memcpy(mac,&value,6);
	}
}


void set_rule_mac_oxm(char *mac,char *oxm)/*OXM是网络序*/
{
	int i = 0;
	for(;i<6;i++)
		mac[i] = oxm[5-i];	
}

void set_rule_ipv6_oxm(char *ipv6,char *oxm)/*OXM是网络序*/
{
	int i = 0;
	for(;i<16;i++)
		ipv6[i] = oxm[15-i];	
}

void oxm2rule(char *dst,char *oxm,int len)
{
	int i = 0;
	for(;i<len;i++)
		dst[i] = oxm[len-1 - i];	
}

/*读规则寄存器*/
u64 openbox_rule_reg_rd(u64 regaddr)
{
//	printf("write rule address is %llx\n",(u64)regaddr);
	fast_reg_wr(FAST_RULE_REG_RADDR,(u64)regaddr<<32);
	return fast_reg_rd(FAST_RULE_REG_VADDR);
}

/*写规则寄存器*/
void openbox_rule_reg_wr(u32 addr,u32 value)
{
	fast_reg_wr(FAST_RULE_REG_WADDR,(u64)addr<<32|value);//原来使用的规则写法，正常	
}

/*读动作寄存器*/
u32 openbox_action_reg_rd(u32 addr)
{
	return 0xFFFFFFFF & fast_reg_rd(FAST_ACTION_REG_ADDR + addr);
}

/*写动作寄存器*/
void openbox_action_reg_wr(u32 addr,u32 value)
{
	fast_reg_wr(FAST_ACTION_REG_ADDR + addr,value);
}

/*往硬件写入一条指定的规则,输入参数为规则索引号*/
void write_rule_normal(int idx,u32 valid)
{
	struct reg_value *value = (struct reg_value *)&table.rules[idx];//将规则转化为32位操作的数据形式
	int i = 0,cnt = sizeof(struct flow)*2/sizeof(struct reg_value);//cnt计算一条规则要写入多个少32位的值
	//写一条指定的规则数据到指定的位置
	table.rules[idx].valid = valid;	
	
	if(valid == 0)
	{
		goto delete_rule;/*删除规则或将规则置无效，仅写完此字段即可*/
	}
	
	/*写此条规则对应的ACTION*/
	openbox_action_reg_wr(idx*8,table.rules[idx].action);

	for(;i<cnt;i++)/*只写key和mask的值*/
	{
		openbox_rule_reg_wr(RULE_LEN*idx + i*sizeof(struct reg_value),(value->v[i]));//前一个计算规则开始的相对位置，后一个为此位置要写入的值
	}
	
	/*写入规则的优先级*/
	openbox_rule_reg_wr(RULE_LEN*idx + cnt*sizeof(table.rules[idx].priority),table.rules[idx].priority);
delete_rule:
	/*写入规则的有效位*/
	openbox_rule_reg_wr(RULE_LEN*idx + (cnt+1)*sizeof(table.rules[idx].valid),table.rules[idx].valid);	
}

/*2017/06/01 BV算法暂不支持新版本流表配置（支持IPv6的新流表项设计，其主要原因是新流表项中输入端口为4位
 而目前实现的BV版本，其最小切分粒度为8位，故端口号设置可能会存在问题）*/
void write_rule_bv(int idx,u32 valid)
{
	struct reg_value *value = (struct reg_value *)&table.rules[idx].key;//将规则转化为32位操作的数据形式
	int i = 0,oft = 0;
	u32 op_idx = 0,mask_value = 0;
	u8 *mask = NULL;
	
	//写一条指定的规则数据到指定的位置	
	
	/*写规则内容*/
	for(;i<sizeof(table.rules[idx].key)/sizeof(struct reg_value);i++)/*把ACTION的位置留出来，不配置在此处*/
	{
		openbox_rule_reg_wr(i*sizeof(struct reg_value),value->v[i]);//前一个计算规则开始的相对位置，后一个为此位置要写入的值
	}
	
	oft = i;
	/*写掩码*/
	mask = (u8 *)&table.rules[idx].mask;	
	for(i=0;i<sizeof(table.rules[idx].mask)/sizeof(*mask);i++)
	{
		if(mask[i] == 0xFF)
		{
			mask_value |= 1<<(i%32);/*1表示有效*/
		}
		else if(mask[i] != 0x00)
		{
			printf("Mask[%d]:%02X!(Must be 0xFF or 0x00)",i,mask[i]);
			exit(0);
		}
		if(i % 32 == 31)
		{
			openbox_rule_reg_wr(4*oft++,mask_value);
			mask_value = 0;
		}
	}
	if(valid == 1)
	{
		op_idx = 0x1;/*1：添加，3：删除*/
	}
	else if(valid == 0)
	{
		op_idx = 0x3;/*1：添加，3：删除*/
	}

	oft = (sizeof(table.rules[idx].key)+sizeof(table.rules[idx].mask))/sizeof(struct reg_value);
	op_idx |= idx<<4;/*4-19位表示规则ID*/
	openbox_rule_reg_wr(oft*4,op_idx);/*写规则ID和操作码*/
	
	//写此条规则对应的ACTION
	openbox_action_reg_wr(idx*8,table.rules[idx].action);

	//软件规则标志更新
	table.rules[idx].valid = valid;

	/*BV算法实现里暂时没有做valid有效位标记和优先级字段，
	 *其优先级根据流表项的ID来判断，ID值较小的优先级高*/
}

void write_rule(int idx,u32 valid)
{
#ifndef LOOKUP_BV
	write_rule_normal(idx,valid);
#else
	write_rule_bv(idx,valid);
#endif
}

/*
 * 判断规则是否已经存在,根据规则的MD5计算值比较,MD5计算是根据key+mask+priority三个字段计算的
 */
int rule_exists(struct fast_rule *rule)
{
	int i = 0;
	for(i=0;i<FAST_RULE_CNT;i++)/*MD5值为16字节*/
	{
		if(table.rules[i].valid == 1 &&
		   table.rules[i].md5[0] == rule->md5[0] &&
		   table.rules[i].md5[1] == rule->md5[1] &&
		   table.rules[i].md5[2] == rule->md5[2] &&
		   table.rules[i].md5[3] == rule->md5[3] 
		   )
			return i;
	}
	return -1;/*表示不存在，存在返回表的索引*/
}

int rule_exists_add(struct fast_rule *rule,int *idx)
{
	int i = 0;
	for(i=0;i<FAST_RULE_CNT;i++)/*MD5值为16字节*/
	{
		if(table.rules[i].valid == 1)
		{
			if(table.rules[i].md5[0] == rule->md5[0] &&
			   table.rules[i].md5[1] == rule->md5[1] &&
			   table.rules[i].md5[2] == rule->md5[2] &&
			   table.rules[i].md5[3] == rule->md5[3] 
			   )
				return i;
		}
		else if(*idx == -1)
		{
			*idx = i;/*记录第一个可用规则位置*/			
		}
	}
	return -1;/*表示不存在，存在返回表的索引*/
}

/*新增一条规则,要求用户输入完整的规则数据结构,包括规则字段,掩码和相应动作 
 * 返回值为存储当前规则的索引值
 */
int fast_add_rule(struct fast_rule *rule)
{
	int idx = -1;
	if(rule_exists_add(rule,&idx) == -1 && idx != -1)
	{
		table.rules[idx] = *rule;//将用户规则存入当前索引的规则位置		
		table.cnt++;//规则数目增加一条
		write_rule(idx,1);/*将此规则写入硬件,将规则置为有效状态*/
	}
	else
	{
		return -E_RULE_INDEX_OVERFLOW;
	}
	return idx;//返回当前规则索引
}

/*修改一条指定位置的规则
 * 返回值与修改索引相等表示修改成功,其他值表示修改失败
 */
int fast_modify_rule(struct fast_rule *rule,int idx)
{
	if(idx >= 0 && idx < FAST_RULE_CNT)
	{
		table.rules[idx] = *rule;//将用户规则存入当前索引的规则位置		
		write_rule(idx,1);//将此规则重新写入硬件,将规则置为有效状态*/
		return idx;
	}
	return -E_RULE_INDEX_OVERFLOW;
}

/*删除一条指定的规则 
 * 返回值与删除索引相等表示删除成功,其他值表示删除失败
 */
int fast_del_rule(int idx)
{
	if(idx >= 0 && idx < FAST_RULE_CNT)
	{
		table.rules[idx] = zero_rule;//将零规则存入当前索引的规则位置		
		write_rule(idx,0);//将此规则重新写入硬件,简化实现方式可以只更新规则的有效标志位
		return idx;
	}
	return -E_RULE_INDEX_OVERFLOW;
}

/*打印软件缓存的规则数据*/
void print_sw_rule(void)
{
	u32 i = 0,j = 0,cnt = sizeof(struct flow)*2/sizeof(struct reg_value) + 3;/*3指优先级、有效位和动作*/	
	struct reg_value *value;

	printf("--------------------------------xxx----------------------------------\n");
	for(i=0;i<FAST_RULE_CNT;i++)
	{
		value = (struct reg_value *)&table.rules[i];
		printf("0x%04X  ",i);
		for(j=0;j<cnt*4;j++)
		{
			printf("%02X",*((u8 *)value+j));
			if(j % 64 == 63)printf("\n--------");
		}
		printf("\n");
	}
}

/*打印硬件存储的规则数据,每个数据都需要从硬件寄存器读返回*/
void print_hw_rule(void)
{
	u32 i = 0,j = 0,cnt = sizeof(struct flow)*2/sizeof(struct reg_value) + 2;	/*2指优先级和有效位，动作要单独读*/
	u32 value = 0;
	printf("-----------------Default Action:0x%X-----------------------------\n",openbox_action_reg_rd(FAST_DEFAULT_RULE_ADDR));
	for(;i<FAST_RULE_CNT;i++)
	{
		printf("0x%04X  ",i);
		for(j=0;j<cnt;j++)
		{
			value = 0xFFFFFFFF & openbox_rule_reg_rd(RULE_LEN*i + j*4);
			printf("%08X ",be32toh(value));
			if(j % 16 == 15)printf("\n--------");
		}
		printf("Action:0x%X",openbox_action_reg_rd(i*8));
		printf("\n");		
	}
	printf("-----------------Default Action:0x%X-----------------------------\n",openbox_action_reg_rd(FAST_DEFAULT_RULE_ADDR));
}

/*从硬件读取一条指定的规则数据,数据存储在用户输入的rule数据结构中
 * 返回值与读规则索引相等表示读成功,其他值表示读取失败
 */
int read_hw_rule(struct fast_rule *rule,int index)
{	
	if(index < 0 || index > FAST_RULE_CNT-1)
	{
		printf("index Error!\n");
		return -1;
	}
	else
	{
		int j = 0,cnt = sizeof(struct flow)*2/sizeof(struct reg_value) + 3;/*3指优先级、有效位和动作*/
		struct reg_value *value = (struct reg_value *)rule;
		for(j=0;j<cnt;j++)
		{
			value->v[j] = be32toh(0xFFFFFFFF & openbox_rule_reg_rd(RULE_LEN*index + j*4));
		}
		return index;
	}
}

/*初始化规则库,将硬件规则存储空间清零*/
void init_rule(u32 default_action)
{
	u32 i = 0,j = 0,cnt = sizeof(struct flow)*2/sizeof(struct reg_value) + 2;/*2指优先级和有效位*/

	memset(&table,0,sizeof(struct rule_table));
	memset(&zero_rule,0,sizeof(struct fast_rule));
#ifndef LOOKUP_BV
	for(;i<FAST_RULE_CNT;i++)
	{
		for(j=0;j<cnt;j++)
		{
			openbox_rule_reg_wr(RULE_LEN*i + j*4,0);//将每条规则数据清零
		}
		openbox_action_reg_wr(i*8,0);	//将规则清零
	}
#else
	/*BV算法不需要将表空间写一次零，可通过表复位操作实现清零，暂未支持*/
#endif
	//给硬件配置默认规则	
	openbox_action_reg_wr(FAST_DEFAULT_RULE_ADDR,default_action);
	FAST_DBG("librule version:%s,Default Action:0x%X\n",RULE_VERSION,default_action);
}
