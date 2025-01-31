// include/config/server_config.h
#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

/* Connection Limits */
#define NUMBER_OF_USERS 10
#define SOCKETS_PER_BUCKET 2
#define USERS_PER_SOCKET 5

/* Network Configuration */
#define MAIN_SOCKET_PORT 8080
#define USER_SOCKET_PORT_START 8081

/* Security Settings */
#define MAX_ATTEMPTS 5
#define BLOCK_DURATION 300
#define BACKOFF_MULTIPLIER 2

/* Buffer Sizes */
#define MAX_BUFFER_SIZE 1024
#define MAX_USERNAME_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define DEFAULT_RECV_BUFFER 8192   /* 8KB receive buffer */
#define DEFAULT_SEND_BUFFER 8192   /* 8KB send buffer */
#define MAX_MESSAGE_SIZE 4096      /* Maximum message size */
#define MIN_BUFFER_SIZE 1024       /* Minimum buffer allocation */

/* Socket Configuration */
#define DEFAULT_BACKLOG 1          /* Single connection backlog (dedicated sockets) */
#define MAX_BACKLOG_SIZE 5         /* Listen queue size */
#define MAX_EVENTS 32              /* Max epoll events to handle at once */

/* Timeouts */
#define CONNECTION_TIMEOUT 100      /* Seconds before inactive connection dropped */
#define EPOLL_TIMEOUT 100          /* MS to wait for epoll events */

/* Status Codes */
#define ERROR -1
#define RUNNING 1
#define STOPPED 0

/* Socket Status */
#define SOCKET_STATUS_UNUSED 0
#define SOCKET_STATUS_ACTIVE 1
#define SOCKET_STATUS_ERROR 2

/* Socket Error Codes */
#define SOCKET_ERROR_NONE 0
#define SOCKET_ERROR_EPOLL 1
#define SOCKET_ERROR_BIND 2
#define SOCKET_ERROR_ACCEPT 3
#define SOCKET_ERROR_LISTEN 4

#endif // SERVER_CONFIG_H