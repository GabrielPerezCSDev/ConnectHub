/*
 * include/socket.h 
 * Structures for individual socket configuration and state
 */
#include <time.h>
#include <pthread.h>
#ifndef SOCKET_H
#define SOCKET_H

/* Socket configuration defaults */
#define DEFAULT_RECV_BUFFER 8192   /* 8KB receive buffer */
#define DEFAULT_SEND_BUFFER 8192   /* 8KB send buffer */
#define DEFAULT_BACKLOG     1      /* Single connection backlog (dedicated sockets) */

/* Socket status flags */
#define SOCKET_STATUS_UNUSED 0
#define SOCKET_STATUS_ACTIVE 1
#define SOCKET_STATUS_ERROR  2

#define PORT_STATUS_FREE      0
#define PORT_STATUS_ACTIVE    1
#define PORT_STATUS_ERROR     2

#define MAX_CONNECTIONS_PER_PORT  1    /* One connection per port */
#define MAX_BACKLOG_SIZE          5    /* Listen queue size */
#define MAX_EVENTS               32    /* Max epoll events to handle at once */

#define CONNECTION_TIMEOUT    100    /* Seconds before inactive connection dropped */
#define EPOLL_TIMEOUT         100    /* MS to wait for epoll events */

#define MAX_MESSAGE_SIZE    4096   /* Maximum message size */
#define MIN_BUFFER_SIZE     1024   /* Minimum buffer allocation */

#define SOCKET_ERROR_NONE     0
#define SOCKET_ERROR_EPOLL    1
#define SOCKET_ERROR_BIND     2
#define SOCKET_ERROR_ACCEPT   3

/*
 * Configuration for socket creation and behavior
 * Defines basic socket parameters and options
 */
typedef struct {
    int recv_buffer_size;   /* Size of receive buffer for each connection */
    int send_buffer_size;   /* Size of send buffer for each connection */
    int backlog;           /* Listen backlog size for incoming connections */
    int reuse_addr;        /* Enable SO_REUSEADDR option */
    int keep_alive;        /* Enable SO_KEEPALIVE option */
} SocketConfig;

/*
 * Manages multiple ports for a single socket
 * Handles port allocation and status tracking
 */
typedef struct {
    int* port_numbers;      /* Array of assigned port numbers */
    int* port_status;       /* Status flags for each port (active/inactive/error) */
    int port_count;         /* Current number of active ports */
    int port_capacity;      /* Maximum number of ports this socket can handle */
} PortManager;

/*
 * Manages multiple client connections
 * Handles epoll and connection tracking
 */
typedef struct {
    int epoll_fd;          /* epoll instance file descriptor */
    int* client_fds;       /* Array of connected client file descriptors */
    int max_connections;   /* Maximum allowed concurrent connections */
} ConnectionManager;

/*
 * Tracks socket and connection statistics
 * Maintains performance and activity metrics
 */
typedef struct {
    unsigned long* bytes_sent_per_port;     /* Bytes sent tracking per port */
    unsigned long* bytes_received_per_port;  /* Bytes received tracking per port */
    int active_connections;                 /* Current number of active connections */
    time_t* last_active_per_port;          /* Last activity timestamp per port */
} SocketStats;

typedef struct {
    SocketConfig config;
    int* port_numbers;
    int port_count;
    int max_connections;
}SocketInitInfo;

/*
 * Main socket structure
 * Manages multiple ports, connections, and associated resources
 * Each socket runs in its own thread handling multiple client connections
 */
typedef struct {
    SocketConfig config;         /* Socket configuration parameters */
    PortManager ports;          /* Port management and tracking */
    ConnectionManager conns;    /* Connection and epoll management */
    SocketStats stats;         /* Performance and activity statistics */
    pthread_t thread_id;       /* ID of thread managing this socket */
    int status;
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
Socket create_socket(const SocketInitInfo);

/*
* Start the socket to be able to be accessed
* @param socket the created socket held by the socket pool
* @return the return value for error notification
*/
int start_socket(Socket* socket);

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