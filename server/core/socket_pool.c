#include "server/socket_pool.h"
#include "server/socket.h"

//create the socket pool with a size and start port
SocketPool* create_socketpool(int num_sockets, int users_per_socket, int start_port){
    printf("Number of sockets for the pool: %d\n", num_sockets);
    SocketPool* pool = (SocketPool*)malloc(sizeof(SocketPool));
    if (!pool) return NULL;
    int max_users = num_sockets * users_per_socket;
    // Initialize socket array
    pool->sockets = (Socket*)malloc(sizeof(Socket) * max_users);
    if (!pool->sockets) {
        free(pool);
        return NULL;
    }

    // Create default config
    SocketConfig scf = create_default_socket_config();

    // Initialize pool fields
    pool->max_users = max_users;
    pool->current_users = 0;
    pool->total_sockets = num_sockets;
    pool->users_per_socket = users_per_socket;
    pool->is_full = 0;
    pool->start_port = start_port;
    pool->status = STOPPED;
    pool->thread_id = -1;


    
    // Create sockets
    int port = start_port;
    for(int i = 0; i < num_sockets; i++) {
        printf("creating socket #%d", (i+1));
        
        
        SocketInitInfo init_info = {
            .config = scf,
            .port_number = port,
            .max_connections = users_per_socket 
        };
        Socket socket = create_socket(init_info);
        printf("}\n\n");
        if (socket.status == SOCKET_STATUS_ERROR) {
            // Cleanup and return NULL
            for(int z = 0; z < i; z++) {
                destroy_socket(&pool->sockets[z]);
            }
            free(pool->sockets);
            free(pool);
            return NULL;
        }

        pool->sockets[i] = socket;
        port++;
    }

    return pool;
}


int start_socketpool(SocketPool* socket_pool) {
    //start each socket 
    int num_sockets = socket_pool->total_sockets;
    for(int i = 0; i < num_sockets; i++){
        printf("\nStarting socket %d\n", (i+1));
        start_socket(&socket_pool->sockets[i]);
    }
    

    return 1;
}

int get_socketpool_status(SocketPool* socket_pool){
    return socket_pool->status;
}

int delete_socketpool(SocketPool* socket_pool) {
    if (!socket_pool) return -1;

    if (socket_pool->sockets) {
        for (int i = 0; i < socket_pool->total_sockets; i++) {
            if (destroy_socket(&socket_pool->sockets[i]) != 0) {
                return -1;
            }
        }
        free(socket_pool->sockets);
    }

    free(socket_pool);
    return 0;
}

/*
int get_socketpool_status(SocketPool* socket_pool){
    return socket_pool->status;
}

void* socket_pool_thread(void* arg) {
   SocketPool* pool = (SocketPool*)arg;
   
   // Thread running status
   pool->status = RUNNING;
   
   while(pool->status == RUNNING) {
       // TODO: Monitor sockets
       // TODO: Accept connections
       // TODO: Handle disconnects
       // TODO: Check for shutdown signal
       
       // Basic sleep to prevent CPU spinning
       usleep(10000);  // 10ms sleep
   }
   
   return NULL;
}

int stop_socketpool(SocketPool* socket_pool) {
    if (!socket_pool || socket_pool->thread_id == -1) return -1;
    
    socket_pool->status = STOPPED;  // Signal thread to stop
    pthread_join(socket_pool->thread_id, NULL);  // Wait for completion
    socket_pool->thread_id = -1;
    socket_pool->status = 0;  // Inactive but not deleted
    
    return 0;
}
*/
