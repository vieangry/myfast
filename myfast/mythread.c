#include "mythread.h"
void* thread()
{
  int i;
  for(i=0;i<3;i++)
  printf("This is a pthread.\n");
}

int dispose(){
    pthread_t id;
	int temp,i;
	if((temp = pthread_create(&id, NULL, thread, NULL)) != 0){
 //comment2
	printf("线程1创建失败!\n");
		return -1;

}
	else
		printf("线程1被创建\n");
	for(i=0;i<3;i++)
	printf("This is the main process.\n");
	pthread_join(id,NULL);
	return 0;
}
