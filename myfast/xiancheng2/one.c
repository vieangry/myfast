#include "one.h"
void test_func()
{
  int sec = 0;
  dispose();
  printf("count is-->%d\n", sec++);
}

void init_sigaction()
{
  struct sigaction act;

  act.sa_handler = test_func; //设置处理信号的函数
  act.sa_flags = 0;

  sigemptyset(&act.sa_mask);
  sigaction(SIGALRM, &act, NULL);//时间到发送SIGROF信号
  printf("init_sigaction successful");
}

void init_time()
{
  struct itimerval val;

  val.it_value.tv_sec = 3; //1秒后启用定时器
  val.it_value.tv_usec = 0;

  val.it_interval = val.it_value; //定时器间隔为1s

  setitimer(ITIMER_REAL, &val, NULL);
  printf("init_time successful");
}
