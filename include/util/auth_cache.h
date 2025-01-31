#ifndef AUTH_CACHE_H
#define AUTH_CACHE_H

#include <stdint.h>
#include <time.h>

/**
 * @file auth_cache.h
 * @brief Implements rate limiting for authentication attempts using a hash table with separate chaining.
 *
 * Tracks failed login attempts by IP and username to prevent brute force attacks. 
 * Provides exponential backoff and temporary blocking for repeated failures.
 */

/** Maximum failed attempts before blocking an IP */
#define MAX_FAILED_ATTEMPTS_IP 10  

/** Maximum failed attempts before blocking a username */
#define MAX_FAILED_ATTEMPTS_USER 5 

/** Time (in seconds) to block a user or IP after exceeding the max failed attempts */
#define BLOCK_DURATION 300  

/** Hash table size for storing authentication attempts */
#define AUTH_HASH_SIZE 128  


/**
 * @struct AuthNode
 * @brief Represents an authentication attempt tracking entry in the hash table.
 */
typedef struct AuthNode {
    char username[64];       /* Username being authenticated */
    char ip_address[46];     /* IPv4 or IPv6 address of the client */
    int failed_attempts_ip;  /* Number of failed attempts for this IP */
    int failed_attempts_user;/* Number of failed attempts for this username */
    time_t last_attempt;     /* Timestamp of the last failed attempt */
    time_t block_until_ip;   /* Time until the IP is unblocked */
    time_t block_until_user; /* Time until the username is unblocked */
    struct AuthNode *next;   /* Pointer to the next node in case of hash collisions */
} AuthNode;

/**
 * @struct AuthCache
 * @brief Hash table for tracking authentication attempts.
 */
typedef struct AuthCache {
    AuthNode *buckets[AUTH_HASH_SIZE]; /* Array of linked lists for separate chaining */
} AuthCache;

/**
 * @brief Creates and initializes an authentication cache.
 * 
 * @return Pointer to a newly allocated `AuthCache`, or NULL on failure.
 */
AuthCache* create_auth_cache(void);

/**
 * @brief Tracks a failed login attempt for a given username and IP address.
 * 
 * @param cache Pointer to the `AuthCache`.
 * @param username Username of the failed authentication attempt.
 * @param ip IP address of the client attempting authentication.
 */
void track_failed_login(AuthCache *cache, const char *username, const char *ip);

/**
 * @brief Checks if a username or IP is currently blocked from authentication.
 * 
 * @param cache Pointer to the `AuthCache`.
 * @param username Username being checked.
 * @param ip IP address being checked.
 * @return 1 if the user or IP is blocked, 0 otherwise.
 */
int is_user_blocked(AuthCache *cache, const char *username, const char *ip);

/**
 * @brief Resets authentication failures for a user and IP upon successful authentication.
 * 
 * @param cache Pointer to the `AuthCache`.
 * @param username Username that successfully authenticated.
 * @param ip IP address of the client that authenticated successfully.
 */
void reset_auth_attempts(AuthCache *cache, const char *username, const char *ip);

/**
 * @brief Destroys the authentication cache and frees all associated memory.
 * 
 * @param cache Pointer to the `AuthCache` to be destroyed.
 */
void destroy_auth_cache(AuthCache *cache);
#endif