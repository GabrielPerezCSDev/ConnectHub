

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