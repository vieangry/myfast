#ifndef FEATURE_H_INCLUDED
#define FEATURE_H_INCLUDED

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
struct syn_flood{
  unsigned char data[54];
  unsigned char IPsrc[16];
  unsigned int IPsrcflag;
  unsigned char IPdst[16];
  unsigned int IPdst_synflag;
  unsigned char syn;
  unsigned char ack;
  unsigned char heibaiflag;
  long firsttime;
}

#endif
