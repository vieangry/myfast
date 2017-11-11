#include "listnode.h"

//dIPdst_synflag某目的端syn连接数
Node* find_atacked_dst(Node *pNode){
	int max=0;
	Node *node = pNode;
	Node *nodemax = pNode;//最大数目的端节点
  if(sizeList(pHead)>100){
    while(node->next != NULL){
  		if(node->element.IPdst_synflag>max){
  			max=node->element.IPdst_synflag;
  			nodemax=node;
  		}
  	}
  	return nodemax;
  }
	return NULL;
}
