#include "list.h"

//抽取所有tcp流
//初始化结构体
//有目的ip和无目的ip的
//syn链接和ack链接
//初始化链表 match

unsigned char IPDST[16];//socket填
//填写头部//DOTO
int inittou(unsigned char *datatou){
  struct syn_flood *flood;
  flood->data[]=//
  flood->syn=//
  flood->ack=//
  flood->heibaiflag=0;//0不确定；１正常；-1异常
  flood->firsttime=getcurrenttieme();
  return 0;
}

Node *findIPsrc(Node *pNode,srtruct syn_flood *flood){
  if(pNode==NULL||flood==NULL){
    printf("erre:kong zhi zhen!\n");
    return NULL;
  }
  Node *head=pNode;
  while (head->next!=NULL) {
    if(ipv6_equal(head->IPsrc, flood->IPdst)==0){
      return head;
    }
    else{
      head=head->next;
    }
  }
  Node *pInsert;
  pInsert = (Node *)malloc(sizeof(Node));
  memset(pInsert,0,sizeof(Node));
  pInsert->element = flood;
  pInsert->next=NULL;
  head->next=insertnode;
  return insertnode;

}
//初始化节点
int initflood(Node *pNode,srtruct syn_flood *flood){
  int ret1=ipv6_equal(IPDST, flood->IPdst);
  Node *p=NULL;
  if(ret1==0&&(flood->syn==1||flood->ack==1)){
    //只有这两种报文我们才处理
    p=findIPsrc(pNode,flood);
    if(p!=NULL){
      if(p->heibaiflag==-1){
        IPdst_synflag++;
        //drop DOTO
        return 0;
      }else {
        p->IPsrcflag++;
        if(p->syn==1){
          IPdst_synflag++;
          cout_syn_connection(pNode,p);//处理syn太频繁
        }
        else if(p->ack==1){
          p->heibaiflag=1;
          //let go DOTO
          return 0;
        }
      }

    }

  }
  else
  //正常转发DOTO
  return 0;
}

//某个syn链接超时处理，即：一条链接过了很长时间都没有收到第三次握手
int timeout(Node *pNode){
//应该用多线程来做吧
}

//IPDST 未定义
//unsigned char IPDST[16]="本端口ip";//初始化DOTO
int match(Node *pNode,srtruct syn_flood *flood){
  int ret1=ipv6_equal(IPDST, flood->IPdst);
  if(ret1==0&&(flood->syn==1||flood->ack==1)){

    //long nowtime =getcurrenttieme()；
    cout_syn_connection(pNode,flood);
    return 0;
  }
  return -1;
}

int cout_syn_connection(Node *pNode,struct syn_flood *flood1){
	if(flood1!=NULL&&flood1!=NULL){
		IPsrcflag++;
    if(flood1.hiebaiflag==0) {
			if(IPsrcflag==1){
        flood1.fisttime=getcurrenttieme();
      //let go
      return 0;
			}else if(IPsrcflag==2){
				flood1.secondtime=getcurrenttieme();
				if(flood1.secondtime-flood1.firsttime<3){
					flood1.heibaiflag=-1;
					//drop
          return 0;
				}else{
          //let go DOTO
          return 0;
        }
			}else if(IPsrcflag==3){
				flood1.3time=getcurrenttieme();
				if(flood1.3time-flood1.firsttime<6){
					flood1.heibaiflag=-1;
					//drop
          return 0;
				}else{
          //let go DOTO
          return 0;
        }
			}else if(IPsrcflag==4){
				flood1.4time=getcurrenttieme();
				if(flood1.4time-flood1.firsttime<12){
					flood1.heibaiflag=-1;
					//drop
          return 0;
				}else{
          //let go DOTO
          return 0;
        }

			}else if(IPsrcflag==5){
				flood1.5time=getcurrenttieme();
				if(flood1.5time-flood1.firsttime<24){
					flood1.heibaiflag=-1;
					//drop
				}else{
          //let go DOTO
          return 0;
        }
			}else {
        //drop
      }

		}else{
      //let go DOTO
    }
	}else//空指针
    return-1;
}
