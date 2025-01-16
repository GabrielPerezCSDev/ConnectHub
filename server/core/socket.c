

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

void* socket_thread_function(void* arg) {
    Socket* sock = (Socket*)arg;
    struct epoll_event events[MAX_EVENTS];

    while (sock->status == SOCKET_STATUS_ACTIVE) {
        int nfds = epoll_wait(sock->conns.epoll_fd, events, MAX_EVENTS, EPOLL_TIMEOUT);
        
        if (nfds < 0) {
            if (errno == EINTR) continue;  
            sock->status = SOCKET_STATUS_ERROR;
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == sock->socket_fd) {
                // Handle new connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                
                int client_fd = accept(sock->socket_fd, 
                                     (struct sockaddr*)&client_addr, 
                                     &client_len);
                
                if (client_fd < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    continue;
                }

                // First message should contain session key
                uint32_t received_key;
                ssize_t bytes_read = read(client_fd, &received_key, sizeof(uint32_t));
                if (bytes_read == sizeof(uint32_t)) {
                    // Find matching session key in our clients array
                    int valid = 0;
                    int slot = -1;
                    for (int j = 0; j < sock->conns.max_connections; j++) {
                        if (sock->conns.clients[j].session_key == received_key && 
                            sock->conns.clients[j].fd == -1) {
                            valid = 1;
                            slot = j;
                            break;
                        }
                    }

                    if (valid) {
                        // Add to epoll
                        struct epoll_event ev;
                        ev.events = EPOLLIN;
                        ev.data.fd = client_fd;
                        if (epoll_ctl(sock->conns.epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                            close(client_fd);
                            continue;
                        }

                        // Update client slot
                        sock->conns.clients[slot].fd = client_fd;
                        sock->conns.clients[slot].last_active = time(NULL);
                        sock->conns.current_connections++;

                        char response[] = "Connection accepted\n";
                        write(client_fd, response, strlen(response));
                        printf("Client connected with valid session key on port %d\n", sock->port);
                    } else {
                        char response[] = "Invalid session key\n";
                        write(client_fd, response, strlen(response));
                        close(client_fd);
                    }
                }
            } else {
                // Handle messages from existing clients
                int client_fd = events[i].data.fd;
                char buffer[MAX_MESSAGE_SIZE];
                
                ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    // Update last active time
                    for (int j = 0; j < sock->conns.max_connections; j++) {
                        if (sock->conns.clients[j].fd == client_fd) {
                            sock->conns.clients[j].last_active = time(NULL);
                            break;
                        }
                    }
                    
                    // Echo message back for now
                    write(client_fd, buffer, bytes_read);
                } else if (bytes_read == 0 || (bytes_read < 0 && errno != EAGAIN)) {
                    // Client disconnected or error
                    for (int j = 0; j < sock->conns.max_connections; j++) {
                        if (sock->conns.clients[j].fd == client_fd) {
                            sock->conns.clients[j].fd = -1;
                            sock->conns.current_connections--;
                            break;
                        }
                    }
                    epoll_ctl(sock->conns.epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                }
            }
        }
    }

    return NULL;
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

    // Create the socket thread
    if (pthread_create(&sock->thread_id, NULL, 
                      socket_thread_function, sock) != 0) {
        printf("Failed to create socket thread\n");
        close(sock->conns.epoll_fd);
        close(sock->socket_fd);
        sock->status = SOCKET_STATUS_ERROR;
        return -1;
    }


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