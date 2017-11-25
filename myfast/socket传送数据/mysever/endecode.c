#include "endecode.h"
#include <string.h>
#include <stdio.h>

int encode(int a[10], char *pkt) {
	int i = 0;
	char *ptr=pkt;
	while(a[i]!=0)
    {
        memcpy(ptr,&a[i], sizeof(int));
        ptr+=sizeof(int);
        i++;
    }
	return i;
}

void decode(char *pkt,int b[]) {
	char *ptr = pkt;
	int i;
	while(ptr!=NULL){
        memcpy(&b[i++], ptr, sizeof(int));
		ptr += sizeof(int);
	}

}
