#include "server/router.h"
#include <stdio.h>
#include <math.h>
#include <arpa/inet.h>

uint32_t generate_session_key()
{
    uint32_t key;
    FILE *urandom = fopen("/dev/urandom", "r");
    if (urandom)
    {
        fread(&key, sizeof(uint32_t), 1, urandom);
        fclose(urandom);
        return key;
    }
    return 0; // Or handle error differently
}

// Set bucket as full
void set_bucket_full(Router *router, int bucket_index)
{
    router->bucket_status[bucket_index / 8] |= (1 << (bucket_index % 8));
}

// Set bucket as not full
void set_bucket_empty(Router *router, int bucket_index)
{
    router->bucket_status[bucket_index / 8] &= ~(1 << (bucket_index % 8));
}

// Check if bucket is full
int is_bucket_full(Router *router, int bucket_index)
{
    return (router->bucket_status[bucket_index / 8] & (1 << (bucket_index % 8))) != 0;
}

// Allocate the bit array
void init_bucket_status(Router *router)
{
    int num_bytes = (router->num_buckets + 7) / 8; // Round up to nearest byte
    router->bucket_status = calloc(num_bytes, sizeof(uint8_t));
}

Router *create_router(UserDB *user_db)
{
    if (!user_db)
        return NULL;
    printf("creating the router \n");
    Router *router = (Router *)malloc(sizeof(Router));
    router->user_db = user_db;
    if (router == NULL)
    {
        printf("Router allocation failed\n");
        return NULL;
    }

    RouterConfig rcf = {
        NUMBER_OF_USERS,
        SOCKETS_PER_BUCKET,
        USERS_PER_SOCKET,
        MAIN_SOCKET_PORT,
        USER_SOCKET_PORT_START,
    };

    router->config = rcf;

    // First create default socket config
    SocketConfig socket_config = create_default_socket_config();

    // Create router socket
    RouterSocket main_socket = create_router_socket(
        socket_config,
        router->config.router_port // or MAIN_SOCKET_PORT
    );

    router->socket = main_socket;

    // generate the SocketBuckets
    int num_buckets = ceil((double)NUMBER_OF_USERS / (USERS_PER_SOCKET * SOCKETS_PER_BUCKET));
    router->num_buckets = num_buckets;
    printf("number of buckets %d\n", num_buckets);

    // Calculate number of bytes needed (round up to nearest byte)
    int num_bytes = (num_buckets + 7) / 8;
    router->bucket_status = (uint8_t *)calloc(num_bytes, sizeof(uint8_t));

    if (!router->bucket_status)
    {
        printf("Failed to allocate bucket status array\n");
        return NULL;
    }

    // Initialize all buckets as empty
    for (int i = 0; i < num_buckets; i++)
    {
        set_bucket_empty(router, i);
    }

    router->socket_pool = (SocketPool *)malloc(sizeof(SocketPool) * num_buckets);

    if (router->socket_pool == NULL)
    {
        printf("Memory allocation for the socket pool failed\n");
    }

    int port = USER_SOCKET_PORT_START;
    for (int i = 0; i < num_buckets; i++)
    {
        printf("\n\nGenerating bucket %d: \n", (i + 1));
        router->socket_pool[i] = *create_socketpool(SOCKETS_PER_BUCKET, USERS_PER_SOCKET, port);
        port += SOCKETS_PER_BUCKET * USERS_PER_SOCKET;
        if (port > (NUMBER_OF_USERS + USER_SOCKET_PORT_START))
        {
            break;
        }
        printf("Finished generating bucket %d with the final port %d \n\n", (i + 1), port);
    }

    router->user_cache = create_user_cache();
    if (!router->user_cache)
    {
        printf("Error genererating user cache\n");
    }

    return router;
}

int assign_user_socket(Router *router, uint32_t session_key)
{

    // Need to find an open bucket first
    int open_index = -1;
    for(int i = 0; i < router->num_buckets; i++){
        if(!is_bucket_full(router, i)){
            open_index = i;
            break;
        }
    }

    if(open_index == -1){
        printf("Server is at capacity (no open buckets)");
        return -1;
    }
    // After open bucket found then delegate socket assignment to the socket pool bucket
    int port_number = find_open_socket(&router->socket_pool[open_index]);
    return port_number;
}

int handle_authentication(Router *router, int client_fd, const char *username, const char *password)
{
    if (!router || !username || !password)
        return -1;

    // First check if user is already logged in
    if (has_user(router->user_cache, username)) {
        char response[256];
        int existing_port = get_user_port(router->user_cache, username);
        uint32_t session_key = get_user_session(router->user_cache, username);
        snprintf(response, sizeof(response), 
            "User already logged in\nPort: %d\nSession key: %u\n",
            existing_port, session_key);
        write(client_fd, response, strlen(response));
        
        return -1;
    }

    // If not logged in, authenticate credentials
    if (authenticate_user(router->user_db, username, password) == DB_SUCCESS)
    {
        if(handle_new_connection(router, username) == 1) {
            int new_port = get_user_port(router->user_cache, username);
            uint32_t session_key = get_user_session(router->user_cache, username);
            
            char response[256];
            snprintf(response, sizeof(response), 
                "Authentication successful\nAssigned to port: %d\nSession key: %u\n",
                new_port, session_key);
            write(client_fd, response, strlen(response));
            
            printf("Successfully authenticated and handled new user to a socket\n");
            return 1;
        }
        char error[] = "Authentication successful but failed to assign port\n";
        write(client_fd, error, strlen(error));
        return -1;
    }
    else
    {
        char response[] = "Authentication failed: Invalid username or password\n";
        write(client_fd, response, strlen(response));
        close(client_fd);
        return -1;
    }
}

int handle_registration(Router *router, int client_fd, const char *username, const char *password)
{
    if (!router || !username || !password)
        return -1;

    if (create_user(router->user_db, username, password) == DB_SUCCESS)
    {
        char response[] = "Registration successful\n";
        write(client_fd, response, strlen(response));
        return 1;
    }
    else
    {
        char response[] = "Registration failed\n";
        write(client_fd, response, strlen(response));
        return -1;
    }
}

void *router_socket_thread(Router *router)
{
    struct epoll_event events[MAX_EVENTS];

    while (router->socket.status == SOCKET_STATUS_ACTIVE)
    {
        int nfds = epoll_wait(router->socket.epoll_fd, events, MAX_EVENTS, EPOLL_TIMEOUT);

        if (nfds < 0)
        {
            if (errno == EINTR)
                continue;
            router->socket.status = SOCKET_STATUS_ERROR;
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == router->socket.socket_fd)
            {
                // Accept new connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                int client_fd = accept(router->socket.socket_fd,
                                       (struct sockaddr *)&client_addr,
                                       &client_len);

                if (client_fd < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        break;
                    }
                    continue;
                }

                // Set socket to non-blocking
                int flags = fcntl(client_fd, F_GETFL, 0);
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                // Add new client to epoll
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(router->socket.epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
                {
                    printf("Failed to add client to epoll\n");
                    close(client_fd);
                    continue;
                }

                printf("[Router] New connection from %s:%d (fd: %d)\n",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port),
                       client_fd);

                router->socket.connections_handled++;
            }
            else
            {
                // Handle data from clients
                int client_fd = events[i].data.fd;
                char buffer[1024];
                ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

                if (bytes_read > 0)
                {
                    buffer[bytes_read] = '\0';

                    char command[5];
                    char username[32];
                    char password[64];

                    if (sscanf(buffer, "%4s %31s %63s", command, username, password) == 3)
                    {
                        if (strcmp(command, "AUTH") == 0)
                        {
                            handle_authentication(router, client_fd, username, password);
                        }
                        else if (strcmp(command, "REG") == 0)
                        {
                            handle_registration(router, client_fd, username, password);
                        }
                        else
                        {
                            char response[] = "Unknown command\n";
                            write(client_fd, response, strlen(response));
                        }
                    }
                    else
                    {
                        char response[] = "Invalid command format. Use: AUTH username password or REG username password\n";
                        write(client_fd, response, strlen(response));
                    }
                }
                else if (bytes_read == 0)
                {
                    // Client disconnected
                    printf("[Router] Client on fd %d disconnected\n", client_fd);
                    epoll_ctl(router->socket.epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                }
            }
        }
    }

    return NULL;
}

int start_router(Router *router)
{
    printf("\n\n Starting the router \n");
    if (!router)
        return -1;

    printf("Start the main socket");
    SocketConfig main_socket_cfg = create_default_socket_config();

    // Create and start router socket
    RouterSocket r_socket = create_router_socket(main_socket_cfg, MAIN_SOCKET_PORT);
    if (start_router_socket(&r_socket) < 0)
    { // Added error checking
        printf("Failed to start main socket\n");
        return -1;
    }

    router->socket = r_socket;
    // Create thread for router socket handling
    if (pthread_create(&router->main_socket_thread, NULL,
                       (void *(*)(void *))router_socket_thread, router) != 0)
    {
        printf("Failed to create router socket thread\n");
        // Should clean up the socket here
        close(router->socket.socket_fd);
        close(router->socket.epoll_fd);
        return -1;
    }

    /* This needs more implementation (will be done later)*/
    for (int i = 0; i < router->num_buckets; i++)
    {
        printf("\nStarting bucket %d:\n", (i + 1));
        if (start_socketpool(&router->socket_pool[i]) < 1)
        {
            return -1; // error
        }
    }

    return 1;
}

void shut_down_router(Router *router)
{
    if (!router)
        return;

    printf("\nInitiating router shutdown...\n");

    // First shut down the main router socket
    router->socket.status = SOCKET_STATUS_UNUSED; // Signal thread to stop

    // Wait for router thread to complete
    if (router->main_socket_thread)
    {
        pthread_join(router->main_socket_thread, NULL);
        printf("Router main socket thread terminated\n");
    }

    // Close router socket file descriptors
    if (router->socket.epoll_fd >= 0)
    {
        close(router->socket.epoll_fd);
        router->socket.epoll_fd = -1;
    }
    if (router->socket.socket_fd >= 0)
    {
        close(router->socket.socket_fd);
        router->socket.socket_fd = -1;
    }

    // Shutdown all socket pools in each bucket
    for (int i = 0; i < router->num_buckets; i++)
    {
        printf("Shutting down socket pool bucket %d\n", i + 1);
        delete_socketpool(&router->socket_pool[i]);
    }

    if (router->user_cache)
    {
        destroy_user_cache(router->user_cache);
        router->user_cache = NULL;
    }

    if (router->bucket_status)
    {
        free(router->bucket_status);
        router->bucket_status = NULL;
    }

    printf("Router shutdown complete\n");
}

int handle_new_connection(Router *router, const char *username)
{
    // create the session key for the user
    uint32_t session_key = generate_session_key();
    printf("session key generated: %u\n", session_key);
    int port_number = assign_user_socket(router, session_key);
    if(port_number == -1){
        printf("could not assign user a socket....");
        return -1;
    }

    //add the user to the cache 
    add_user(router->user_cache, username, port_number, session_key);
    return 1;
}