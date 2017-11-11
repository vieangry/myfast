#include "list.h"

//抽取所有tcp流
//初始化结构体
//有目的ip和无目的ip的
//syn链接和ack链接
//初始化链表 match

//初始化节点
int initflood(Node *pNode,srtruct syn_flood *flood){

  return 0;
}


//IPDST 未定义
//unsigned char IPDST[16]="本端口ip";//初始化DOTO
int match(Node *pNode,srtruct syn_flood *flood){
  int ret1=ipv6_equal(IPDST, flood->IPdst);
  if(ret1==0&&(flood->syn==1||flood->ack==1)){
    //只有这两种报文我们才处理
    //long nowtime =getcurrenttieme()；
    cout_syn_connection(flood);
    return 0;
  }
  return -1;
}

int cout_syn_connection(struct syn_flood *flood1){
	if(flood1!=NULL){
		IPsrcflag++;
		if(flood1.hiebaiflag==-1){
			//drop
			}
		else if(flood1.hiebaiflag==0) {
			if(IPsrcflag==1){
			//let go
			flood1.fisttime=getcurrenttieme();
			}else if(IPsrcflag==2){
				flood1.secondtime=getcurrenttieme();
				if(flood1.secondtime-flood1.firsttime<3){
					flood1.heibaiflag=-1;
					//drop
				}
			}else if(IPsrcflag==3){
				flood1.3time=getcurrenttieme();
				if(flood1.3time-flood1.firsttime<6){
					flood1.heibaiflag=-1;
					//drop
				}
			}else if(IPsrcflag==4){
				flood1.4time=getcurrenttieme();
				if(flood1.4time-flood1.firsttime<12){
					flood1.heibaiflag=-1;
					//drop
				}
			}else if(IPsrcflag==5){
				flood1.5time=getcurrenttieme();
				if(flood1.5time-flood1.firsttime<24){
					flood1.heibaiflag=-1;
					//drop
				}
			}else //drop

		}
			return 0;
	}
}
