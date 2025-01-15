/*
 * include/socket.h 
 * Structures for individual socket configuration and state
 */
#include <time.h>
#include <sys/socket.h>     // For socket(), bind(), listen(), accept()
#include <netinet/in.h>     // For struct sockaddr_in
#include <string.h>         // For memset()
#include <unistd.h>         // For close()
#include <fcntl.h>          // For fcntl() - setting non-blocking mode
#include <sys/epoll.h>      // For epoll_create1(), epoll_ctl(), epoll_wait()
#include <pthread.h>        // For pthread_create() and thread handling
#include <errno.h>          // For errno and error constants

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
#define SOCKET_ERROR_LISTEN   4

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

typedef struct {
    uint32_t session_key;  // Random 32-bit session identifier
    int fd;                // Connection file descriptor
    time_t last_active;    // Last activity timestamp
} ClientConnection;
/*
 * Manages multiple client connections
 * Handles epoll and connection tracking
 */
typedef struct {
    int epoll_fd;          /* epoll instance file descriptor */
    ClientConnection* clients;  /* Array of client connections */
    int max_connections;   /* Maximum allowed concurrent connections */
    int current_connections; /* Current number of active connections */
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
    int port_number;
    int max_connections;
}SocketInitInfo;

/*
 * Main socket structure
 * Manages multiple ports, connections, and associated resources
 * Each socket runs in its own thread handling multiple client connections
 */
typedef struct {
    SocketConfig config;         /* Socket configuration parameters */
    ConnectionManager conns;    /* Connection and epoll management */
    SocketStats stats;         /* Performance and activity statistics */
    pthread_t thread_id;       /* ID of thread managing this socket */
    int port;
    int socket_fd;
    int status;
    int error;
} Socket;

/*
 * Main router socket structure
 * Handles initial connections and routing
 */
typedef struct {
    SocketConfig config;         /* Socket configuration parameters */
    int socket_fd;              /* Main socket file descriptor */
    int epoll_fd;
    int thread_id;
    time_t last_used;
    int port;                  /* Router port number */
    int status;               /* Socket status */
    unsigned long connections_handled;  /* Total connections processed */
} RouterSocket;


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

RouterSocket create_router_socket(const SocketConfig config, int port);
/*
* Start the socket to be able to be accessed
* @param socket the created socket held by the socket pool
* @return the return value for error notification
*/
int start_socket(Socket* sock);

/*
 * Start the main router socket
 * Returns -1 on error, 1 on success
 */
int start_router_socket(RouterSocket* router_socket);

/*
 * Clean up and free a socket
 * @param socket Socket to destroy
 */
int destroy_socket(Socket* sock);

/*
 * Update socket activity timestamp
 * @param socket Socket to update
 */
void update_socket_activity(Socket* sock);

/*
 * Update socket statistics
 * @param socket Socket to update
 * @param bytes_sent Number of bytes sent
 * @param bytes_received Number of bytes received
 */
void update_socket_stats(Socket* sock, unsigned long bytes_sent, unsigned long bytes_received);

int is_socket_full(Socket* sock);

#endif /* SOCKET_H */