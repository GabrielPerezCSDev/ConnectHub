#include "server/router.h"
#include <stdlib.h>

int main(){

    //create the router
    Router* router = create_router();

    if(start_router(router) < 1){
        printf("Failed to start router exiting...\n");
        return 1;
    }
    return 1;
}