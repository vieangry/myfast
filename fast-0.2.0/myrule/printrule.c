#include "../include/fast.h"
#include <malloc.h>
#include <stdio.h>
int main(int argc,char* argv[])
{
	fast_init_hw(0, 0);
	print_hw_rule();
  print_sw_rule();
	struct fast_rule rule1;
	rule1=malloc(sizeof(fast_rule));
	memset(&rule1,0,sizeof(struct fast_rule));
	inet_pton(int af, const char *src, void *dst);
	rule1.key.ipv6.src="2001:250:4401:2000:6254:7e81:ed31:5353";
	rule1.mask.ipv6.src=0xFFFFFFFF;
	fast_add_rule(&rule1);
	printf("*****************\n");
	print_hw_rule();
	return 0;
}
