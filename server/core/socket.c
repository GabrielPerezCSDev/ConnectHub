

#include <stdlib.h>
#include "server/socket.h"
#include <string.h>
#include <stdio.h>

/* Socket configuration defaults */
#define DEFAULT_RECV_BUFFER 8192   /* 8KB receive buffer */
#define DEFAULT_SEND_BUFFER 8192   /* 8KB send buffer */
#define DEFAULT_BACKLOG 1          /* Single connection backlog (dedicated sockets) */

/* Socket status flags */
#define SOCKET_STATUS_UNUSED 0
#define SOCKET_STATUS_ACTIVE 1
#define SOCKET_STATUS_ERROR  2


/* Function declarations */

/*
 * Creates default configuration for a socket
 * @return SocketConfig with default values
 */
SocketConfig create_default_socket_config(void){
    SocketConfig scf = {
        DEFAULT_RECV_BUFFER,
        DEFAULT_SEND_BUFFER,
        DEFAULT_BACKLOG,
        1,
        1,
    };

    return scf;
}

Socket create_socket(const SocketInitInfo socket_init_info){
    
    //Copy of port numbers array for ownership
    int* port_numbers = (int*) malloc(sizeof(int) * socket_init_info.port_count);
    if (!port_numbers) {
        // Handle error
        Socket error_socket = {0};  // Zero initialize all fields
        error_socket.status = SOCKET_STATUS_ERROR;
        return error_socket /* error socket */;
    }
    memcpy(port_numbers, socket_init_info.port_numbers, sizeof(int) * socket_init_info.port_count);
    
    //PortManager defintion 
    int* port_status = (int*) malloc(sizeof(int) * socket_init_info.port_count);
    
    if (!port_status) {
        free(port_numbers);
        Socket error_socket = {0};  // Zero initialize all fields
        error_socket.status = SOCKET_STATUS_ERROR;
        return error_socket /* error socket */;
    }

    for(int i = 0; i < socket_init_info.port_count;i++){
        port_status[i] = SOCKET_STATUS_UNUSED;
    }
    
    PortManager pmgr = {
        socket_init_info.port_numbers,
        port_status,
        0,
        socket_init_info.port_count,
    };

    /*
    * Connection manager intialization
    */

   int* client_fds = (int*) malloc(sizeof(int) * socket_init_info.max_connections);
    if (!client_fds) {
        // Handle error
        free(port_numbers);
        free(port_status);
        Socket error_socket = {0};  // Zero initialize all fields
        error_socket.status = SOCKET_STATUS_ERROR;
        return error_socket /* error socket */;
    }
    
    // Initialize all client FDs to -1 (indicating no connection)
    for(int i = 0; i < socket_init_info.max_connections; i++) {
        client_fds[i] = -1;
    }

    ConnectionManager cmgr = {
        -1,                              // epoll_fd (-1 until started)
        client_fds,                      // Array for client FDs
        socket_init_info.max_connections // Max connections allowed
    };

    // Allocate arrays based on port count
unsigned long* bytes_sent = (unsigned long*) malloc(sizeof(unsigned long) * socket_init_info.port_count);
unsigned long* bytes_received = (unsigned long*) malloc(sizeof(unsigned long) * socket_init_info.port_count);
time_t* last_active = (time_t*) malloc(sizeof(time_t) * socket_init_info.port_count);

if (!bytes_sent || !bytes_received || !last_active) {
    // Handle error - free any successful allocations
    free(port_numbers);
    free(port_status);
    free(client_fds);
    free(bytes_sent);
    free(bytes_received);
    free(last_active);
    Socket error_socket = {0};  // Zero initialize all fields
        error_socket.status = SOCKET_STATUS_ERROR;
        return error_socket /* error socket */;
}

// Initialize all statistics to 0/inactive
for(int i = 0; i < socket_init_info.port_count; i++) {
    bytes_sent[i] = 0;
    bytes_received[i] = 0;
    last_active[i] = 0;  // 0 timestamp indicates never active
}

SocketStats stats = {
    bytes_sent,
    bytes_received,
    0,                    // No active connections initially
    last_active
};

Socket socket = {
    socket_init_info.config,
    pmgr,
    cmgr,
    stats,
    0,
    SOCKET_STATUS_UNUSED,
};

return socket;

}

RouterSocket create_router_socket(const SocketConfig config, int port) {
    RouterSocket router_socket = {0};  // Zero initialize all fields
    
    // Set initial values
    router_socket.config = config;
    router_socket.socket_fd = -1;      // Not started yet
    router_socket.port = port;
    router_socket.status = SOCKET_STATUS_UNUSED;
    router_socket.connections_handled = 0;

    return router_socket;
}

int start_socket(Socket* socket){
    if(!socket) return -1;
    //create the socket fd 

    for(int i = 0; i < socket->ports.port_capacity;i++){
        printf("Starting the port %d\n", socket->ports.port_numbers[i]);
    }
    return 1;
}

int start_main_socket(RouterSocket* router_socket) {
   if (!router_socket) return -1;

   // Create socket
   router_socket->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
   if (router_socket->socket_fd < 0) {
       router_socket->status = SOCKET_STATUS_ERROR;
       return -1;
   }

   // Set socket options
   int opt = 1;
   if (setsockopt(router_socket->socket_fd, SOL_SOCKET, SO_REUSEADDR, 
                  &opt, sizeof(opt)) < 0) {
       close(router_socket->socket_fd);
       router_socket->status = SOCKET_STATUS_ERROR;
       return -1;
   }

   // Prepare address structure
   struct sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
   addr.sin_port = htons(router_socket->port);

   // Bind socket to port
   if (bind(router_socket->socket_fd, (struct sockaddr*)&addr, 
            sizeof(addr)) < 0) {
       close(router_socket->socket_fd);
       router_socket->status = SOCKET_STATUS_ERROR;
       return -1;
   }

   // Start listening
   if (listen(router_socket->socket_fd, router_socket->config.backlog) < 0) {
       close(router_socket->socket_fd);
       router_socket->status = SOCKET_STATUS_ERROR;
       return -1;
   }

   router_socket->status = SOCKET_STATUS_ACTIVE;
   return 1;
}


int destroy_socket(Socket* socket){
    //TODO destroy the socket

    return 1;
}