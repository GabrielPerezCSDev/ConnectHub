#include "config/config_validator.h"
#include "config/server_config.h"

#include "limits.h"
#include <stdio.h>

ConfigValidationResult validate_server_config(void) {
    //Connection limits
    if (NUMBER_OF_USERS <= 0 || NUMBER_OF_USERS >= INT_MAX) {
        return CONFIG_INVALID_USER_CAPACITY;
    }

    if (SOCKETS_PER_BUCKET <= 0) {
        return CONFIG_INVALID_BUCKET_SIZE;
    }

    if (USERS_PER_SOCKET <= 0) {
        return CONFIG_INVALID_SOCKET_USERS;
    }

    // Network Configuration
    if (MAIN_SOCKET_PORT < 1024 || MAIN_SOCKET_PORT > 65535) {
        return CONFIG_INVALID_PORT_RANGE;
    }
    if (USER_SOCKET_PORT_START <= MAIN_SOCKET_PORT) {
        return CONFIG_INVALID_PORT_RANGE;
    }
    // Check if enough ports available for all sockets
    int required_ports = (NUMBER_OF_USERS / USERS_PER_SOCKET) + 1;
    if (USER_SOCKET_PORT_START + required_ports > 65535) {
        return CONFIG_INVALID_PORT_SPACING;
    }
    
    // Buffer Sizes
    if (MAX_MESSAGE_SIZE < MIN_BUFFER_SIZE) {
        return CONFIG_INVALID_BUFFER_SIZE;
    }
    if (MAX_MESSAGE_SIZE > DEFAULT_RECV_BUFFER) {
        return CONFIG_INVALID_MESSAGE_SIZE;
    }
    if (MAX_USERNAME_LENGTH <= 0 || MAX_USERNAME_LENGTH > 255) {
        return CONFIG_INVALID_USERNAME_LENGTH;
    }
    if (MAX_PASSWORD_LENGTH <= 0 || MAX_PASSWORD_LENGTH > 255) {
        return CONFIG_INVALID_USERNAME_LENGTH;
    }

     // Timeouts
    if (CONNECTION_TIMEOUT <= 0 || CONNECTION_TIMEOUT > 3600) {  // Max 1 hour
        return CONFIG_INVALID_TIMEOUT_VALUE;
    }
    if (EPOLL_TIMEOUT <= 0 || EPOLL_TIMEOUT > 1000) {  // Max 1 second
        return CONFIG_INVALID_TIMEOUT_VALUE;
    }

    // Security Settings
    if (MAX_ATTEMPTS <= 0) {
        return CONFIG_INVALID_ATTEMPT_LIMIT;
    }
    if (BLOCK_DURATION <= 0) {
        return CONFIG_INVALID_BLOCK_DURATION;
    }
    if (BACKOFF_MULTIPLIER <= 1) {
        return CONFIG_INVALID_BACKOFF_VALUE;
    }

    return CONFIG_VALID;
}

const char* get_config_error_message(ConfigValidationResult result) {
    switch(result) {
        case CONFIG_VALID:
            return "Configuration is valid";
        case CONFIG_INVALID_USER_CAPACITY:
            return "Invalid number of users (must be > 0 and < INT_MAX)";
        case CONFIG_INVALID_BUCKET_SIZE:
            return "Invalid sockets per bucket (must be > 0)";
        case CONFIG_INVALID_SOCKET_USERS:
            return "Invalid users per socket (must be > 0)";
        case CONFIG_INVALID_PORT_RANGE:
            return "Invalid port range (must be between 1024 and 65535)";
        case CONFIG_INVALID_PORT_SPACING:
            return "Not enough available ports for required sockets";
        case CONFIG_INVALID_BUFFER_SIZE:
            return "Maximum message size cannot be smaller than minimum buffer size";
        case CONFIG_INVALID_MESSAGE_SIZE:
            return "Maximum message size cannot exceed receive buffer size";
        case CONFIG_INVALID_USERNAME_LENGTH:
            return "Invalid username/password length (must be between 1 and 255)";
        case CONFIG_INVALID_TIMEOUT_VALUE:
            return "Invalid timeout values (connection: max 1hr, epoll: max 1sec)";
        case CONFIG_INVALID_ATTEMPT_LIMIT:
            return "Invalid maximum attempts value (must be > 0)";
        case CONFIG_INVALID_BLOCK_DURATION:
            return "Invalid block duration (must be > 0)";
        case CONFIG_INVALID_BACKOFF_VALUE:
            return "Invalid backoff multiplier (must be > 1)";
        default:
            return "Unknown configuration error";
    }
}

void print_server_config(void) {
    printf("\nServer Configuration Summary:\n");
    printf("===========================\n");
    
    printf("\nConnection Limits:\n");
    printf("  Max Users: %d\n", NUMBER_OF_USERS);
    printf("  Sockets per Bucket: %d\n", SOCKETS_PER_BUCKET);
    printf("  Users per Socket: %d\n", USERS_PER_SOCKET);
    
    printf("\nNetwork Configuration:\n");
    printf("  Main Router Port: %d\n", MAIN_SOCKET_PORT);
    printf("  User Socket Start Port: %d\n", USER_SOCKET_PORT_START);
    
    printf("\nSecurity Settings:\n");
    printf("  Max Login Attempts: %d\n", MAX_ATTEMPTS);
    printf("  Block Duration: %d seconds\n", BLOCK_DURATION);
    printf("  Backoff Multiplier: %d\n", BACKOFF_MULTIPLIER);
    
    printf("\nBuffer Configuration:\n");
    printf("  Max Message Size: %d bytes\n", MAX_MESSAGE_SIZE);
    printf("  Min Buffer Size: %d bytes\n", MIN_BUFFER_SIZE);
    printf("  Receive Buffer: %d bytes\n", DEFAULT_RECV_BUFFER);
    printf("  Send Buffer: %d bytes\n", DEFAULT_SEND_BUFFER);
    
    printf("\nTimeout Settings:\n");
    printf("  Connection Timeout: %d seconds\n", CONNECTION_TIMEOUT);
    printf("  Epoll Timeout: %d ms\n", EPOLL_TIMEOUT);
    
    printf("\n===========================\n\n");
}