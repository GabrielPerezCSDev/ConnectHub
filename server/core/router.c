#include "server/router.h"
#include <stdio.h>
#include <math.h>
#include <arpa/inet.h>


static inline void cleanup_client_connection(ClientConnectionData *client_data, int epoll_fd)
{
    if (!client_data)
        return;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_data->client_fd, NULL);
    close(client_data->client_fd);
    free(client_data);
}


struct SessionKeyResult generate_session_key()
{
    struct SessionKeyResult result = {
        0,
        SESSION_KEY_SUCCESS};

    FILE *urandom = fopen("/dev/urandom", "r");

    if (!urandom)
    {
        result.error = SESSION_KEY_FILE_ERROR;
        return result;
    }

    size_t read_items = fread(&result.key, sizeof(uint32_t), 1, urandom);

    if (read_items != 1)
    {
        result.error = SESSION_KEY_READ_ERROR;
    }

    fclose(urandom);

    return result; // Or handle error differently
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
    {
        errno = EINVAL;
        return NULL;
    }
    printf("creating the router \n");
    Router *router = (Router *)calloc(1, sizeof(Router));
    if (!router)
    {
        errno = ENOMEM;
    }

    router->user_db = user_db;
    if (router == NULL)
    {
        printf("Router allocation failed\n");
        return NULL;
    }

    // clean slate for init
    router->user_db = user_db;
    router->num_buckets = 0;
    router->bucket_status = NULL;
    router->socket_pool = NULL;
    router->user_cache = NULL;
    router->auth_cache = NULL;

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

    // Create user cache
    router->user_cache = create_user_cache();
    if (!router->user_cache)
    {
        printf("Error generating user cache\n");
        free(router->socket_pool);
        free(router->bucket_status);
        free(router);
        return NULL;
    }

    // Create auth cache
    router->auth_cache = create_auth_cache();
    if (!router->auth_cache)
    {
        printf("Error generating authentication cache\n");
        destroy_user_cache(router->user_cache);
        free(router->socket_pool);
        free(router->bucket_status);
        free(router);
        return NULL;
    }

    return router;
}

int assign_user_socket(Router *router, uint32_t session_key)
{
    if (!router)
    {
        errno = EINVAL;
        return -1;
    }

    // Need to find an open bucket first
    int retries = 3;
    while (retries--)
    {
        int open_index = -1;
        for (int i = 0; i < router->num_buckets; i++)
        {
            if (!is_bucket_full(router, i))
            {
                open_index = i;
                break;
            }
        }

        if (open_index == -1)
        {
            if (retries > 0)
            {
                // Wait briefly before retry
                usleep(100000); // 100ms
                continue;
            }
            errno = ENOSPC;
            printf("Server is at capacity (no open buckets)");
            return -1;
        }

        // After open bucket found then delegate socket assignment to the socket pool bucket
        int port_number = find_open_socket(&router->socket_pool[open_index], session_key);
        if (port_number > 0)
        {
            return port_number;
        }
    }
    return -1;
}

int handle_authentication(Router *router, int client_fd, const char *username, const char *password, const char *ip)
{
    if (!router || !username || !password)
        return -1;

    // Check if the IP or username is rate-limited
    if (is_user_blocked(router->auth_cache, username, ip))
    {
        char response[] = "Too many failed attempts. Try again later.\n";
        write(client_fd, response, strlen(response));
        close(client_fd);
        return -1;
    }

    // First check if user is already logged in
    if (has_user(router->user_cache, username))
    {
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
        // Reset failed login attempts (remove from AuthCache)
        reset_auth_attempts(router->auth_cache, username, ip);

        if (handle_new_connection(router, username) == 1)
        {
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
        // Track failed login attempt
        track_failed_login(router->auth_cache, username, ip);

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
    if (!router)
    {
        errno = EINVAL;
        return NULL;
    }

    struct epoll_event events[MAX_EVENTS];
    int retry_count = 0;
    const int MAX_RETRIES = 3;

    while (router->socket.status == SOCKET_STATUS_ACTIVE)
    {
        int nfds = epoll_wait(router->socket.epoll_fd, events, MAX_EVENTS, EPOLL_TIMEOUT);

        if (nfds < 0)
        {
            if (errno == EINTR)
                continue;

            retry_count++;
            if (retry_count > MAX_RETRIES)
            {
                router->socket.status = SOCKET_STATUS_ERROR;
                break;
            }
            continue;
        }
        retry_count = 0;

        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == router->socket.socket_fd)
            {
                // Accept new connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                ClientConnectionData *client_data = malloc(sizeof(ClientConnectionData));
                if (!client_data)
                {
                    perror("Failed to allocate memory for client data");
                    continue;
                }

                client_data->client_fd = accept(router->socket.socket_fd,
                                                (struct sockaddr *)&client_addr,
                                                &client_len);

                if (client_data->client_fd < 0)
                {
                    free(client_data);
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        break;
                    }
                    continue;
                }
                // Store client IP
                inet_ntop(AF_INET, &client_addr.sin_addr, client_data->client_ip, INET_ADDRSTRLEN);

                // Set socket to non-blocking
                int flags = fcntl(client_data->client_fd, F_GETFL, 0);
                fcntl(client_data->client_fd, F_SETFL, flags | O_NONBLOCK);

                // Add new client to epoll
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.ptr = client_data;
                if (epoll_ctl(router->socket.epoll_fd, EPOLL_CTL_ADD, client_data->client_fd, &ev) < 0)
                {
                    printf("Failed to add client to epoll\n");
                    close(client_data->client_fd);
                    free(client_data);
                    continue;
                }
                printf("[Router] New connection from %s:%d (fd: %d)\n",
                       client_data->client_ip, ntohs(client_addr.sin_port), client_data->client_fd);

                router->socket.connections_handled++;
            }
            else
            {
                // Handle data from clients
                ClientConnectionData *client_data = (ClientConnectionData *)events[i].data.ptr;
                char buffer[1024];
                ssize_t bytes_read = read(client_data->client_fd, buffer, sizeof(buffer) - 1);

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
                            handle_authentication(router, client_data->client_fd, username, password, client_data->client_ip);
                        }
                        else if (strcmp(command, "REG") == 0)
                        {
                            handle_registration(router, client_data->client_fd, username, password);
                        }
                        else
                        {
                            char response[] = "Unknown command\n";
                            write(client_data->client_fd, response, strlen(response));
                        }
                    }
                    else
                    {
                        char response[] = "Invalid command format. Use: AUTH username password or REG username password\n";
                        write(client_data->client_fd, response, strlen(response));
                    }
                }
                else if (bytes_read == 0)
                {
                    // Client disconnected
                    printf("[Router] Client on fd %d disconnected\n", client_data->client_fd);
                    cleanup_client_connection(client_data, router->socket.epoll_fd);
                }
            }
        }
    }

    for (int i = 0; i < MAX_EVENTS; i++)
    {
        if (events[i].data.ptr)
        {
            ClientConnectionData *client_data = (ClientConnectionData *)events[i].data.ptr;
            if (client_data)
            {
                cleanup_client_connection(client_data, router->socket.epoll_fd);
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

static void cleanup_router_partial(Router *router)
{
    if (!router)
        return;

    if (router->bucket_status)
        free(router->bucket_status);
    if (router->socket_pool)
    {
        for (int i = 0; i < router->num_buckets; i++)
        {
            if (router->socket_pool[i].sockets)
            {
                delete_socketpool(&router->socket_pool[i]);
            }
        }
    }

    free(router->socket_pool);

    if (router->user_cache)
        destroy_user_cache(router->user_cache);

    free(router);
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
    struct SessionKeyResult result = generate_session_key();
    // Handle session key generation errors
    if (result.error != SESSION_KEY_SUCCESS)
    {
        if (result.error == SESSION_KEY_FILE_ERROR)
        {
            fprintf(stderr, "[ERROR] Failed to open /dev/urandom for reading session key.\n");
        }
        else if (result.error == SESSION_KEY_READ_ERROR)
        {
            fprintf(stderr, "[ERROR] Failed to read session key from /dev/urandom.\n");
        }
        return -1; // Indicate failure
    }

    printf("[INFO] Session key generated: %u\n", result.key);

    int port_number = assign_user_socket(router, result.key);
    if (port_number == -1)
    {
        fprintf(stderr, "[ERROR] Could not assign user a socket.\n");
        return -1;
    }

    add_user(router->user_cache, username, port_number, result.key);
    return 1;
}