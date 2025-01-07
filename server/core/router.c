#include "server/router.h"
#include <stdio.h>
#include <math.h>

Router* create_router(){
    printf("creating the router \n");
    Router* router = (Router*) malloc(sizeof(Router));
    if(router == NULL){
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

    // Initialize all ports as inactive
    for(int i = 0; i < NUMBER_OF_USERS; i++){
        router->active_ports[i] = 0;
    }

    //generate the SocketBuckets 
    int num_buckets = ceil((double)  NUMBER_OF_USERS / (SOCKETS_PER_BUCKET * USERS_PER_SOCKET));
    router->num_buckets = num_buckets;
    printf("number of buckets %d\n", num_buckets); 

    router->socket_pool = (SocketPool*) malloc(sizeof(SocketPool) * num_buckets);

    if(router->socket_pool == NULL){
        printf("Memory allocation for the socket pool failed\n");
    }

    int port = USER_SOCKET_PORT_START;
    for(int i = 0; i < num_buckets; i++){
        printf("\nGenerating bucket %d: \n", (i+1));
        router->socket_pool[i] = *create_socketpool(SOCKETS_PER_BUCKET, USERS_PER_SOCKET, port);
        port += SOCKETS_PER_BUCKET * USERS_PER_SOCKET;
        if(port > (NUMBER_OF_USERS + USER_SOCKET_PORT_START)){
            break;
        }
        printf("Finished generating bucket %d with the final port %d \n\n", (i+1), port);
    }

    return router;
}


int start_router(Router* router){
    printf("\n\n Starting the router \n");
    if (!router) return -1;

    for(int i = 0; i < router->num_buckets; i++){
        printf("\nStarting bucket %d:\n", (i+1));
        if(start_socketpool(&router->socket_pool[i]) < 1){
            return -1; //error
        }
    }

    return 1;
}