#include "server/router.h"
#include "db/user_db.h"
#include "db/db_config.h"
#include "config/config_validator.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

// Function to ensure data directory exists
static void ensure_data_directory() {
    struct stat st = {0};
    if (stat("./data", &st) == -1) {
        mkdir("./data", 0700);
    }
}

int main() {
    //ensure user configurations are legit 
    ConfigValidationResult result = validate_server_config();
    if (result != CONFIG_VALID) {
        fprintf(stderr, "Configuration validation failed: %s\n", 
            get_config_error_message(result));
        return 1;
    }

    print_server_config();
    // Ensure data directory exists
    ensure_data_directory();

    // Initialize database
    UserDB* user_db = init_user_db(DEFAULT_DB_PATH);
    if (!user_db) {
        printf("Failed to initialize database, exiting...\n");
        return 1;
    }

    // Create the router
    Router* router = create_router(user_db);
    if (!router) {
        printf("Failed to create router, exiting...\n");
        close_user_db(user_db);
        return 1;
    }

    if(start_router(router) < 1) {
        printf("Failed to start router exiting...\n");
        close_user_db(user_db);
        free(router);
        return 1;
    }

    printf("\nServer is running. Press 'Q' to quit.\n");
    
    char input;
    while(1) {
        input = getchar();
        if(input == 'Q' || input == 'q') {
            printf("\nInitiating server shutdown...\n");
            shut_down_router(router);
            free(router);
            close_user_db(user_db);
            break;
        }
    }

    printf("Server shutdown complete.\n");
    return 0;
}