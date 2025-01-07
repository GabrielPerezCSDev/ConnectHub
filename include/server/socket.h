/*
 * include/socket.h 
 * Structures for individual socket configuration and state
 */
#include <time.h>
#ifndef SOCKET_H
#define SOCKET_H

/* Socket configuration defaults */
#define DEFAULT_RECV_BUFFER 8192   /* 8KB receive buffer */
#define DEFAULT_SEND_BUFFER 8192   /* 8KB send buffer */
#define DEFAULT_BACKLOG 1          /* Single connection backlog (dedicated sockets) */

/* Socket status flags */
#define SOCKET_STATUS_UNUSED 0
#define SOCKET_STATUS_ACTIVE 1
#define SOCKET_STATUS_ERROR  2

/*
 * Configuration for an individual socket
 * Contains configurable parameters for socket creation
 */
typedef struct {
    int recv_buffer_size;   /* Size of receive buffer */
    int send_buffer_size;   /* Size of send buffer */
    int backlog;           /* Listen backlog size */
    int reuse_addr;        /* Whether to set SO_REUSEADDR */
    int keep_alive;        /* Whether to set SO_KEEPALIVE */
} SocketConfig;

/*
 * Structure representing a socket
 * Contains socket configuration and current state
 */
typedef struct {
    SocketConfig config;       /* Socket configuration */
    int port_number;          /* Assigned port number */
    int socket_fd;            /* Socket file descriptor */
    int status;              /* Current socket status */
    unsigned long bytes_sent; /* Statistics - bytes sent */
    unsigned long bytes_recv; /* Statistics - bytes received */
    time_t last_active;      /* Last activity timestamp */
} Socket;

/* Function declarations */

/*
 * Creates default configuration for a socket
 * @return SocketConfig with default values
 */
SocketConfig create_default_socket_config(void);

/*
 * Initialize a socket with given configuration
 * @param config Configuration to use
 * @param port_number Port to bind to
 * @return Initialized Socket or NULL on error
 */
Socket create_socket(const SocketConfig config, int port_number);

/*
 * Clean up and free a socket
 * @param socket Socket to destroy
 */
int destroy_socket(Socket* socket);

/*
 * Update socket activity timestamp
 * @param socket Socket to update
 */
void update_socket_activity(Socket* socket);

/*
 * Update socket statistics
 * @param socket Socket to update
 * @param bytes_sent Number of bytes sent
 * @param bytes_received Number of bytes received
 */
void update_socket_stats(Socket* socket, unsigned long bytes_sent, unsigned long bytes_received);

#endif /* SOCKET_H */