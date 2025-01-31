// include/config/config_validator.h
#ifndef CONFIG_VALIDATOR_H
#define CONFIG_VALIDATOR_H

typedef enum {
    CONFIG_VALID = 0,
    // Connection Limits Errors
    CONFIG_INVALID_USER_CAPACITY,    // NUMBER_OF_USERS vs socket capacity
    CONFIG_INVALID_BUCKET_SIZE,      // SOCKETS_PER_BUCKET must be > 0
    CONFIG_INVALID_SOCKET_USERS,     // USERS_PER_SOCKET must be > 0

    // Network Configuration Errors
    CONFIG_INVALID_PORT_RANGE,       // Port number validation
    CONFIG_INVALID_PORT_SPACING,     // Enough ports for all sockets

    // Buffer Size Errors
    CONFIG_INVALID_BUFFER_SIZE,      // Buffer size relationships
    CONFIG_INVALID_MESSAGE_SIZE,     // Message size validation
    CONFIG_INVALID_USERNAME_LENGTH,  // Username/password length checks
    
    // Timeout Errors
    CONFIG_INVALID_TIMEOUT_VALUE,    // Timeout values must be reasonable
    
    // Security Setting Errors
    CONFIG_INVALID_ATTEMPT_LIMIT,    // MAX_ATTEMPTS > 0
    CONFIG_INVALID_BLOCK_DURATION,   // BLOCK_DURATION > 0
    CONFIG_INVALID_BACKOFF_VALUE     // BACKOFF_MULTIPLIER > 1
} ConfigValidationResult;

/**
 * @brief Validates all server configuration values
 * @return ConfigValidationResult indicating validation status
 */
ConfigValidationResult validate_server_config(void);

/**
 * @brief Return an error messafe for failed config
 * @return Return the char poitner with the error message
 */
const char* get_config_error_message(ConfigValidationResult result);

void print_server_config(void);

#endif