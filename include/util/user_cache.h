/**
 * @file user_cache.h
 * @author Gabriel Perez
 * @brief 
 * A hashmap data strcture to hold active users on the server and their respevite ports 
 * <char[], int> key value format
 * @version 0.1
 * @date 2025-01-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */


#ifndef USER_CACHE_H
#define USER_CACHE_H

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define HASH_SIZE 1024  // Size of hash table, power of 2
#define MAX_USERNAME 32 // Max length of username
#define HASH_MASK (HASH_SIZE - 1)  // For fast modulo

typedef struct UserNode {
    char username[MAX_USERNAME];
    int port;
    uint32_t session_key;     // For verification
    time_t last_active;       // For timeout management
    struct UserNode* next;    // For collision handling
} UserNode;

typedef struct {
    UserNode* buckets[HASH_SIZE];
    int size;                 // Current number of entries
} UserCache;

// Core functions
UserCache* create_user_cache(void);
void destroy_user_cache(UserCache* cache);

// Operations
int add_user(UserCache* cache, const char* username, int port, uint32_t session_key);
int remove_user(UserCache* cache, const char* username);
int get_user_port(UserCache* cache, const char* username);
uint32_t get_user_session(UserCache* cache, const char* username);
int update_user_activity(UserCache* cache, const char* username);

// Utility functions
int has_user(UserCache* cache, const char* username);
int is_port_in_use(UserCache* cache, int port);
void cleanup_inactive_users(UserCache* cache, time_t timeout);

// Hash function for strings
static inline unsigned int hash_username(const char* username) {
    unsigned int hash = 0;
    while (*username) {
        hash = hash * 31 + *username++;
    }
    return hash & HASH_MASK;
}
#endif