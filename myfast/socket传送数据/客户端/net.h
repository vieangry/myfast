
int send_msg(int sock, const void *buf, int len);

int recv_msg(int sock, void *buf, int len);

int get_server_socket(int port, int maxsock);

int get_client_socket(int port, char *ip);