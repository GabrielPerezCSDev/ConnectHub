
#ifndef SOCKET_POOL_H
#define SOCKET_POOL_H

#include "socket.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define ERROR -1
#define RUNNING 1
#define STOPPED 0

typedef struct {
    int max_users;
    int current_users;
    int is_full;
    int users_per_socket;
    int total_sockets; 
    int start_port;
    Socket* sockets;
    int status;
    pthread_t thread_id;
}SocketPool;

SocketPool* create_socketpool(int num_sockets, int users_per_socket, int start_port);

int start_socketpool(SocketPool* socket_pool);

void* socket_pool_thread(void* arg);

int delete_socketpool(SocketPool* socket_pool);

/*
//delete a socket pool
int delete_socketpool(SocketPool* socket_pool);

// Stop the socket pool (but don't delete)
int stop_socketpool(SocketPool* socket_pool);
*/
#endif