#ifndef LIST_H
#define LIST_H

#include "stdio.h"
#include <stdlib.h>
#include "string.h"
typedef int elemType ;
typedef struct Node{    /* 定义单链表结点类型 */
    elemType element;
    struct Node *next;
}Node;

void creatList(Node *pNode);
Node *initList(Node *pHead);
void printList(Node *pHead);
void clearList(Node *pHead);
int sizeList(Node *pHead);
int isEmptyList(Node *pHead);
elemType getElement(Node *pHead, int pos);
elemType *getElemAddr(Node *pHead, elemType x);
int modifyElem(Node *pNode,int pos,elemType x);
int insertHeadList(Node **pNode,elemType insertElem);
int insertLastList(Node **pNode,elemType insertElem);
int deleteHeadList(Node **pNode);
int deleteLastList(Node **pNode);


/************************************************************************/
/*             以下是关于线性表链接存储（单链表）操作的18种算法        */

/* 1.创建线性表，即置单链表的表头指针为空 */
/* 2.初始化线性表，此函数输入负数终止读取数据*/
/* 3.打印链表，链表的遍历*/
/* 4.清除线性表L中的所有元素，即释放单链表L中所有的结点，使之成为一个空表 */
/* 5.返回单链表的长度 */
/* 6.检查单链表是否为空，若为空则返回１，否则返回０ */
/* 8.从单链表中查找具有给定值x的第一个元素，若查找成功则返回该结点data域的存储地址，否则返回NULL */
/* 10.向单链表的表头插入一个元素 */
/* 11.向单链表的末尾添加一个元素 */
/* .14.从单链表中删除表头结点，并把该结点的值返回，若删除失败则停止程序运行 */
/* .15.从单链表中删除表尾结点并返回它的值，若删除失败则停止程序运行 */
/* .17.从单链表中删除值为x的第一个结点，若删除成功则返回1,否则返回0 */

/************************************************************************/



#endif // LIST_H
