

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
    /*
    * Client Connection default
    */
    ClientConnection* clients = (ClientConnection*) malloc(sizeof(ClientConnection) * socket_init_info.max_connections);
    if(!clients){
        printf("Unsuccessful allocation of memory for clients\n");
        Socket error_socket = {0};  // Zero initialize all fields
        error_socket.status = SOCKET_STATUS_ERROR;
        return error_socket /* error socket */;
    }

    for(int i = 0; i < socket_init_info.max_connections; i++){
        clients[i].fd = -1;                 // No file descriptor
        clients[i].session_key = 0;         // No session key
        clients[i].last_active = 0;         // No activity
    }
    /*
    * Connection manager intialization
    */

    ConnectionManager cmgr = {
        -1,                              // epoll_fd (-1 until started)
        clients,                      // Array for client FDs
        socket_init_info.max_connections, // Max connections allowed
        0,                              // Currently no established connections
    };

    // Allocate arrays based on port count
unsigned long* bytes_sent = (unsigned long*) malloc(sizeof(unsigned long) * socket_init_info.max_connections);
unsigned long* bytes_received = (unsigned long*) malloc(sizeof(unsigned long) * socket_init_info.max_connections);
time_t* last_active = (time_t*) malloc(sizeof(time_t) * socket_init_info.max_connections);

if (!bytes_sent || !bytes_received || !last_active) {
    // Handle error - free any successful allocations
    free(clients);
    free(bytes_sent);
    free(bytes_received);
    free(last_active);
    Socket error_socket = {0};  // Zero initialize all fields
        error_socket.status = SOCKET_STATUS_ERROR;
        return error_socket /* error socket */;
}

// Initialize all statistics to 0/inactive
for(int i = 0; i < socket_init_info.max_connections; i++) {
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
    socket_init_info.config,   // config                    
    cmgr,                      // conns
    stats,                     // stats
    0,                         // thread_id
    socket_init_info.port_number,
    -1,                        // socket_fd
    SOCKET_STATUS_UNUSED,       // status
    SOCKET_ERROR_NONE
};

return socket;

}

RouterSocket create_router_socket(const SocketConfig config, int port) {
    RouterSocket router_socket = {0};  // Zero initialize all fields
    
    // Set initial values
    router_socket.config = config;
    router_socket.socket_fd = -1;      // Not started yet
    router_socket.epoll_fd = -1;       // Not created yet
    router_socket.thread_id = 0;       // No thread yet
    router_socket.port = port;
    router_socket.status = SOCKET_STATUS_UNUSED;
    router_socket.connections_handled = 0;

    return router_socket;
}

int start_socket(Socket* sock) {
    if (!sock) return -1;

    // Create the main socket fd
    sock->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock->socket_fd < 0) {
        printf("Failed to create socket fd\n");
        sock->status = SOCKET_STATUS_ERROR;
        return -1;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(sock->socket_fd, SOL_SOCKET, SO_REUSEADDR, 
                   &opt, sizeof(opt)) < 0) {
        printf("Failed to set socket options\n");
        close(sock->socket_fd);
        sock->status = SOCKET_STATUS_ERROR;
        return -1;
    }

    // Create epoll instance
    sock->conns.epoll_fd = epoll_create1(0);
    if (sock->conns.epoll_fd < 0) {
        printf("Failed to create epoll instance\n");
        close(sock->socket_fd);
        sock->status = SOCKET_STATUS_ERROR;
        return -1;
    }

    
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(sock->port);
        // Bind socket to this port
        if (bind(sock->socket_fd, (struct sockaddr*)&addr, 
                 sizeof(addr)) < 0) {
            printf("Failed to bind port %d\n", sock->port);
            sock->status = SOCKET_STATUS_ERROR;
            sock->error = SOCKET_ERROR_BIND;
            return -1;
        }

        // Start listening on this port
        if (listen(sock->socket_fd, sock->config.backlog) < 0) {
            printf("Failed to listen on port %d\n", sock->port);
            sock->status = SOCKET_STATUS_ERROR;
            sock->error = SOCKET_ERROR_LISTEN;
            return -1;
        }

        // Add to epoll
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sock->socket_fd;
        if (epoll_ctl(sock->conns.epoll_fd, EPOLL_CTL_ADD, 
                      sock->socket_fd, &ev) < 0) {
            printf("Failed to add socket at port %d to epoll\n", sock->port);
            sock->status = SOCKET_STATUS_ERROR;
            sock->error = SOCKET_ERROR_EPOLL;
            return -1;
        }

        printf("Successfully started socket at port %d\n", sock->port);
    // Check if any ports were successfully started
    
    if (sock->error != 0) {
        printf("Faulty socket\n");
        close(sock->conns.epoll_fd);
        close(sock->socket_fd);
        sock->status = SOCKET_STATUS_ERROR;
        return -1;
    }
    sock->status = SOCKET_STATUS_ACTIVE;
    return 1;
}

int start_router_socket(RouterSocket* router_socket) {
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

    // Set non-blocking mode for the socket
    if (fcntl(router_socket->socket_fd, F_SETFL, 
              fcntl(router_socket->socket_fd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        close(router_socket->socket_fd);
        router_socket->status = SOCKET_STATUS_ERROR;
        return -1;
    }

    // Create epoll instance
    router_socket->epoll_fd = epoll_create1(0);
    if (router_socket->epoll_fd < 0) {
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
        close(router_socket->epoll_fd);
        close(router_socket->socket_fd);
        router_socket->status = SOCKET_STATUS_ERROR;
        return -1;
    }

    // Start listening
    if (listen(router_socket->socket_fd, router_socket->config.backlog) < 0) {
        close(router_socket->epoll_fd);
        close(router_socket->socket_fd);
        router_socket->status = SOCKET_STATUS_ERROR;
        return -1;
    }

    // Add the listening socket to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // Edge-triggered mode
    ev.data.fd = router_socket->socket_fd;
    if (epoll_ctl(router_socket->epoll_fd, EPOLL_CTL_ADD, 
                  router_socket->socket_fd, &ev) < 0) {
        close(router_socket->epoll_fd);
        close(router_socket->socket_fd);
        router_socket->status = SOCKET_STATUS_ERROR;
        return -1;
    }
    
    router_socket->status = SOCKET_STATUS_ACTIVE;
    return 1;
}


int destroy_socket(Socket* sock) {

    if (!sock) return -1;

    printf("Destroying socket on port %d\n", sock->port);

    // Close epoll file descriptor
    if (sock->conns.epoll_fd >= 0) {
        close(sock->conns.epoll_fd);
        sock->conns.epoll_fd = -1;
    }

    // Close main socket file descriptor
    if (sock->socket_fd >= 0) {
        close(sock->socket_fd);
        sock->socket_fd = -1;
    }

    // Close all client connections and free resources
    if (sock->conns.clients) {
        // Close any open client connections
        for (int i = 0; i < sock->conns.max_connections; i++) {
            if (sock->conns.clients[i].fd >= 0) {
                close(sock->conns.clients[i].fd);
            }
        }
        // Free the clients array
        free(sock->conns.clients);
        sock->conns.clients = NULL;
    }

    // Reset status flags
    sock->status = SOCKET_STATUS_UNUSED;
    sock->error = SOCKET_ERROR_NONE;
    sock->conns.current_connections = 0;

    printf("Socket destroyed successfully\n");
    return 1;
}

int is_socket_full(Socket* sock){
    if(sock->conns.current_connections < sock->conns.max_connections){
        return 1;
    }

    return -1;
}