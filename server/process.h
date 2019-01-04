#ifndef PROCESS_H
#define PROCESS_H

#include "../common/header.h"
#include "socket.h"
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <stdlib.h>

// structure for message queue 
struct mesg_buffer { 
    long mesg_type; 
    int fd;
};

struct process {
	pid_t pid;
	struct sockaddr_in serv_address;
	size_t addrlen;

	fd_set socket_set; // reader_set
	fd_set writer_set; // writer_set
	fd_set exception_set; // exception_set

	int max_fd;
	struct client_socket_list *sock_list;
};

struct process * init_process(pid_t pid, struct fd_linked_list *socket_fds, struct sockaddr_in serv_address);
struct process * init_processes(int count, struct fd_linked_list *socket_fds, struct sockaddr_in serv_address);

#endif