#include "Connection.h"

Connection::Connection() {
	int i;
	for(i=0; i<12; i++) {
		switch_mac[i] = '\0';
	}
}

Connection::~Connection() {
	
}

struct epoll_event Connection::get_event() {
	return ev;
}

void Connection::set_sock(int sock) {
	this->sock = sock;
}

void Connection::set_event(struct epoll_event ev) {
	this->ev = ev;
}

void Connection::set_mac(char* mac) {
	int i;
	for(i=0; i<12; i++) {
		switch_mac[i] = mac[i];
	}
}
