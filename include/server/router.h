

#ifndef ROUTER_H
#define ROUTER_H
#include "socket.h"
#include "socket_pool.h"
#include <pthread.h>
#include "db/user_db.h"
#include "util/user_cache.h"
#include "util/auth_cache.h"

#define NUMBER_OF_USERS 10
#define SOCKETS_PER_BUCKET 2
#define USERS_PER_SOCKET 5
#define MAIN_SOCKET_PORT 8080
#define USER_SOCKET_PORT_START 8081

#define MAX_ATTEMPTS 5       // Max failed attempts before blocking
#define BLOCK_DURATION 300   // Block for 5 minutes (300 seconds)
#define BACKOFF_MULTIPLIER 2 // Exponential backoff factor

/**
 * @enum SessionKeyError
 * @brief Error codes for session key generation
 *
 * @var SESSION_KEY_SUCCESS   Key was generated successfully
 * @var SESSION_KEY_FILE_ERROR Unable to open /dev/urandom
 * @var SESSION_KEY_READ_ERROR Failed to read from /dev/urandom
 */
typedef enum SessionKeyError
{
    SESSION_KEY_SUCCESS = 0,
    SESSION_KEY_FILE_ERROR,
    SESSION_KEY_READ_ERROR
} SessionKeyError;

/**
 * @struct SessionKeyResult
 * @brief Result structure for session key generation
 *
 * @var key    The generated session key (0 if generation failed)
 * @var error  Error code indicating success/failure reason
 */
struct SessionKeyResult
{
    uint32_t key;
    SessionKeyError error;
};

typedef struct {
    int client_fd;
    char client_ip[INET_ADDRSTRLEN];
} ClientConnectionData;
/**
 * @struct RouterConfig
 * @brief Configuration settings for the router (uses the constants set in this header file)
 */
typedef struct
{
    int max_users;        // Maximum number of concurrent users
    int bucket_size;      // Number of sockets per bucket
    int users_per_socket; // Maximum users per socket
    int router_port;      // Main router listening port
    int start_port;       // Starting port for user sockets
} RouterConfig;

/**
 * @struct Router
 * @brief Main router structure managing socket pools and connections
 */
typedef struct
{
    RouterConfig config;
    uint8_t *bucket_status;       // Bitmap tracking bucket fullness
    SocketPool *socket_pool;      // Array of socket pools
    int num_buckets;              // Total number of socket buckets
    RouterSocket socket;          // Main router socket
    pthread_t main_socket_thread; // Thread handling main socket
    UserDB *user_db;              // User database reference
    UserCache *user_cache;        // Active user session cache
    AuthCache *auth_cache;        // Tracks failed authentication attempts
} Router;

/**
 * @brief Creates and initializes a new router instance
 * @param user_db Pointer to initialized user database
 * @return Pointer to initialized Router or NULL on failure
 */
Router *create_router(UserDB *user_db);

/**
 * @brief Starts the router and its socket pools
 * @param router Pointer to initialized router
 * @return 1 on success, -1 on failure
 */
int start_router(Router *router);

/**
 * @brief Handles new user connections and assigns them to available sockets
 * @param router Pointer to router
 * @param username Username of connecting user
 * @return 1 on success, -1 on failure
 */
int handle_new_connection(Router *router, const char *username);

/**
 * @brief Removes a user connection from the router
 * @param router Pointer to router
 * @param username Username of user to remove
 * @return 1 on success, -1 on failure
 */
int remove_connection(Router *router, const char *username);

/**
 * @brief Gracefully shuts down the router and frees resources
 * @param router Pointer to router to shut down
 */
void shut_down_router(Router *router);

/**
 * @brief Handles user authentication requests
 * @param router Pointer to router
 * @param client_fd Client socket file descriptor
 * @param username Username to authenticate
 * @param password Password to verify
 * @return 1 on success, -1 on failure
 */
int handle_authentication(Router *router, int client_fd, const char *username, const char *password, const char *ip);

/**
 * @brief Handles new user registration requests
 * @param router Pointer to router
 * @param client_fd Client socket file descriptor
 * @param username Username to register
 * @param password Password to set
 * @return 1 on success, -1 on failure
 */
int handle_registration(Router *router, int client_fd, const char *username, const char *password);

/**
 * @brief Generates a new session key
 * @return SessionKeyResult containing key and error status
 */
struct SessionKeyResult generate_session_key(void);

#endif /* ROUTER_H*/