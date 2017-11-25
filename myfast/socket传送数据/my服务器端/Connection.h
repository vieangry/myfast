#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/epoll.h>

class Connection{
	public:
		Connection();
		~Connection();
		
		struct epoll_event get_event();
		void set_sock(int sock);
		void set_event(struct epoll_event ev);
		void set_mac(char* mac);
	protected:
	private:
		int sock;
		char switch_mac[12];
		struct epoll_event ev;
};

#endif
