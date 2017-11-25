#ifndef NET_H_INCLUDED
#define NET_H_INCLUDED
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
int send_msg(int sock, const void *buf, int len);

int recv_msg(int sock, void *buf, int len);

int get_server_socket(int port, int maxsock);

int get_client_socket(int port, char *ip);

#endif // NET_H_INCLUDED
