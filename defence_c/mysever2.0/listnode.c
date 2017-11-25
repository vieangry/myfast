#include "listnode.h"
/* 1.初始化线性表，即置单链表的表头指针为空 */
void creatList(Node *pNode)
{
    pNode = NULL;
    printf("createlist successful\n");
}

/* 2.创建线性表，此函数输入负数终止读取数据*/
Node *initList(Node *pHead)
{
    Node *p1;
    Node *p2;

    p1=p2=(Node *)malloc(sizeof(Node)); //申请新节点
    if(p1 == NULL || p2 ==NULL)
    {
        printf("place failed\n");
        exit(0);
    }
    memset(p1,0,sizeof(Node));

    scanf("%d",&p1->element);    //输入新节点
    p1->next = NULL;         //新节点的指针置为空
    while(p1->element > 0)        //输入的值大于0则继续，直到输入的值为负
    {
        if(pHead == NULL)       //空表，接入表头
        {
            pHead = p1;
        }
        else
        {
            p2->next = p1;       //非空表，接入表尾
        }
        p2 = p1;
        p1=(Node *)malloc(sizeof(Node));    //再重申请一个节点
        if(p1 == NULL || p2 ==NULL)
        {
        printf("place failed\n");
        exit(0);
        }
        memset(p1,0,sizeof(Node));
        scanf("%d",&p1->element);
        p1->next = NULL;
    }
    printf("creatList  successful\n");
    return pHead;           //返回链表的头指针
}

/* 3.打印链表，链表的遍历*/
void printList(Node *pHead)
{
    if(NULL == pHead)   //链表为空
    {
        printf("PrintList函数执行，链表为空\n");
    }
    else
    {
        while(NULL != pHead)
        {
            printf("%d -->",pHead->element);
            pHead = pHead->next;
        }
        printf("\n");
    }
    printf("print successful");
}

/* 4.清除线性表L中的所有元素，即释放单链表L中所有的结点，使之成为一个空表 */
void clearList(Node *pHead)
{
    Node *pNext;            //定义一个与pHead相邻节点

    if(pHead == NULL)
    {
        printf("clearList函数执行，链表为空\n");
        return;
    }
    while(pHead->next != NULL)
    {
        pNext = pHead->next;//保存下一结点的指针
        free(pHead);
        pHead = pNext;      //表头下移
    }
    printf("clearList successful\n");
}

/* 5.返回单链表的长度 */
int sizeList(Node *pHead)
{
    int size = 0;

    while(pHead != NULL)
    {
        size++;         //遍历链表size大小比链表的实际长度小1
        pHead = pHead->next;
    }
    printf("sizeList  successful %d \n",size);
    return size;    //链表的实际长度
}

/* 6.检查单链表是否为空，若为空则返回１，否则返回０ */
int isEmptyList(Node *pHead)
{
    if(pHead == NULL)
    {
        printf("isEmptyList函数执行，链表为空\n");
        return 1;
    }
    printf("isEmptyList  successful\n");

    return 0;
}


/* 8.从单链表中查找具有给定值x的第一个元素，若查找成功则返回该结点data域的存储地址，否则返回NULL */
elemType *getElemAddr(Node *pHead, elemType x)
{
    if(NULL == pHead)
    {
        printf("getElemAddr函数执行，链表为空\n");
        return NULL;
    }
    if(x < 0)
    {
        printf("getElemAddr函数执行，给定值X不合法\n");
        return NULL;
    }
    while((pHead->element != x) && (NULL != pHead->next)) //判断是否到链表末尾，以及是否存在所要找的元素
    {
        pHead = pHead->next;
    }
    if((pHead->element != x) && (pHead != NULL))
    {
        printf("getElemAddr函数执行，在链表中未找到x值\n");
        return NULL;
    }
    if(pHead->element == x)
    {
        printf("getElemAddr函数执行，元素 %d 的地址为 0x%x\n",x,&(pHead->element));
    }

    return &(pHead->element);//返回元素的地址
}


/* 10.向单链表的表头插入一个元素 */
int insertHeadList(Node **pNode,elemType insertElem)
{
    Node *pInsert;
    pInsert = (Node *)malloc(sizeof(Node));
    memset(pInsert,0,sizeof(Node));
    pInsert->element = insertElem;
	if(*pNode==NULL){
		*pNode=pInsert;
		pInsert->next = NULL;
    }else{
		pInsert->next = *pNode;
		*pNode = pInsert;

    }
    printf("insertHeadList  successful\n");

    return 1;
}

/* 11.向单链表的末尾添加一个元素 */
int insertLastList(Node **pNode,elemType insertElem)
{

    Node *pInsert;
    Node *pHead;
    Node *pTmp; //定义一个临时链表用来存放第一个节点

    pHead = *pNode;
    pTmp = pHead;

    pInsert = (Node *)malloc(sizeof(Node)); //申请一个新节点
    memset(pInsert,0,sizeof(Node));
    pInsert->element = insertElem;
    pInsert->next=NULL;
    if(*pNode==NULL){
		*pNode=pInsert;
    }else{
		while(pHead->next != NULL)
		{
        pHead = pHead->next;
		}
		pHead->next = pInsert;   //将链表末尾节点的下一结点指向新添加的节点
		*pNode = pTmp;

    }

    printf("insertLastList  successful\n");

    return 1;
}

//从单链表中删除表头结点，并把该结点的值返回，若删除失败则停止程序运行
int deleteHeadList(Node **pNode)
{
    Node *phead;
	if(*pNode==NULL){
		return 0;
    }else{
        phead=*pNode;
		*pNode = (*pNode)->next;
        free(phead);
    }
    printf("deletHeadList  successful\n");

    return 1;
}
//从单链表中删除表尾结点并返回它的值，若删除失败则停止程序运行 */
int deleteLastList(Node **pNode)
{
    Node *pHead,*temp;
    pHead=*pNode;
    temp=pHead;
    if(*pNode==NULL){
		return 0;
    }else if(pHead->next==NULL){
        free(*pNode);
        *pNode==NULL;
    }
    else{
        pHead=pHead->next;
		while(pHead->next != NULL)
		{
		temp=temp->next;
        pHead = pHead->next;
		}
		temp->next=NULL;
		free(pHead);
    }

    printf("deletLastList  successful\n");

    return 1;
}
//-1表示错误；0表示相等；１表示不相等
int ipv6_equal(char *addr1, char *addr2)
{
    int ret = -1;
    int i = 0;
    unsigned char n_addr1[16] = {-1};
    unsigned char n_addr2[16] = {-1};

    if (!addr1) {
        printf("addr1 is NULL\n");
        return -1;
    }

    if (!addr2) {
        printf("addr2 is NULL\n");
        return -1;
    }

    ret = inet_pton(AF_INET6, addr1, &(n_addr1));
    if (ret <= 0 ) {
        if (ret == 0) {
            printf("addr1: Invalid IPv6 address\n");
        }

        return -1;
    }
    ret = inet_pton(AF_INET6, addr2, &(n_addr2));
    if (ret <=0 ) {
        if (ret == 0) {
            printf("addr2: Invalid IPv6 address\n");
        }

        return -1;
    }

    for (i = 0; i < 16; i++) {
        //printf("i: %d, addr1: %u, addr2: %u\n", i, n_addr1[i], n_addr2[i]);
        if (n_addr1[i] != n_addr2[i]) {
            return 1;
        }
    }
    return 0;
}
//获取当前系统时间
long getcurrenttieme(){
	struct timeval tv;
	gettimeofday(&tv,NULL);//tv.tv_sec秒   tv.tv_usec毫秒
	return tv.tv_sec;
}
