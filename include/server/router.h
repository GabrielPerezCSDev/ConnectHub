

#ifndef ROUTER_H
#define ROUTER_H
#include "socket.h"
#include "socket_pool.h"
#include <pthread.h>
#include "db/user_db.h"
#include "util/user_cache.h"

#define NUMBER_OF_USERS 10
#define SOCKETS_PER_BUCKET 2
#define USERS_PER_SOCKET 5
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
    uint8_t* bucket_status;  // Each bit represents a bucket's full status
    SocketPool* socket_pool; //the socket pool for the router
    int num_buckets;
    RouterSocket socket;
    pthread_t main_socket_thread;
    UserDB* user_db;
    UserCache* user_cache;
} Router;

/*
* Create router and populate the fields
*/
Router* create_router(UserDB* user_db);

/*
* Start the router (enable socket pools and sockets)
*/
int start_router(Router* router);

void* router_socket_thread(Router* router);
/*
* New connection for the router so assign it if possible to a socket (not in use) 
*/
int handle_new_connection(Router* router, const char *username);

int remove_connection(Router* router, const char *username);

void shut_down_router(Router* router);

// Authentication related functions
int handle_authentication(Router* router, int client_fd, const char* username, const char* password);
int handle_registration(Router* router, int client_fd, const char* username, const char* password);

#endif /* ROUTER_H*/