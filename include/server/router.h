

#ifndef ROUTER_H
#define ROUTER_H
#include "socket.h"
#include "socket_pool.h"
#include <pthread.h>


#define NUMBER_OF_USERS 1000
#define SOCKETS_PER_BUCKET 10
#define USERS_PER_SOCKET 10
#define MAIN_SOCKET_PORT 8080
#define USER_SOCKET_PORT_START 8081

typedef struct {
    int max_users;          // NUMBER_OF_USERS
    int bucket_size;        // BUCKET_SIZE
    int users_per_socket;   // USERS_PER_SOCKET
    int router_port;        // MAIN_SOCKET_PORT
    int start_port;         // USER_SOCKET_PORT_START
} RouterConfig;

typedef struct{
    RouterConfig config;
    int active_ports[NUMBER_OF_USERS]; //used to track port usage (also implicitly conccurent users)
    SocketPool* socket_pool; //the socket pool for the router
} Router;

/*
* Create router and populate the fields
*/
Router* create_router();

/*
* New connection for the router so assign it if possible to a socket (not in use) 
*/
int handle_new_connection(Router* router);

int remove_connection(Router* router);

void delete_router(Router* router);


#endif /* ROUTER_H*/