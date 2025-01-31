#include "util/auth_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static unsigned int hash_username_ip(const char *username, const char *ip)
{
    unsigned int hash = 5381;
    while (*username)
        hash = ((hash << 5) + hash) + *username++;
    while (*ip)
        hash = ((hash << 5) + hash) + *ip++;
    return hash % AUTH_HASH_SIZE;
}

AuthCache *create_auth_cache(void)
{
    AuthCache *cache = malloc(sizeof(AuthCache));
    if (!cache)
        return NULL;
    memset(cache->buckets, 0, sizeof(cache->buckets));
    return cache;
}

void track_failed_login(AuthCache *cache, const char *username, const char *ip)
{
    if (!cache || !username || !ip)
        return;

    unsigned int index = hash_username_ip(username, ip);
    AuthNode *current = cache->buckets[index];

    while (current)
    {
        if (strcmp(current->username, username) == 0 && strcmp(current->ip_address, ip) == 0)
        {
            current->failed_attempts_ip++;
            current->failed_attempts_user++;
            current->last_attempt = time(NULL);

            // Apply exponential backoff (2^N seconds)
            int delay = 1 << current->failed_attempts_ip;
            sleep(delay);

            if (current->failed_attempts_ip >= MAX_FAILED_ATTEMPTS_IP)
            {
                current->block_until_ip = time(NULL) + BLOCK_DURATION;
            }

            if (current->failed_attempts_user >= MAX_FAILED_ATTEMPTS_USER)
            {
                current->block_until_user = time(NULL) + BLOCK_DURATION;
            }
            return;
        }
        current = current->next;
    }

    // Add new entry
    AuthNode *new_node = malloc(sizeof(AuthNode));
    if (!new_node)
        return;

    strncpy(new_node->username, username, sizeof(new_node->username) - 1);
    strncpy(new_node->ip_address, ip, sizeof(new_node->ip_address) - 1);
    new_node->username[63] = '\0';
    new_node->ip_address[45] = '\0';
    new_node->failed_attempts_ip = 1;
    new_node->failed_attempts_user = 1;
    new_node->last_attempt = time(NULL);
    new_node->block_until_ip = 0;
    new_node->block_until_user = 0;
    new_node->next = cache->buckets[index];
    cache->buckets[index] = new_node;
}

int is_user_blocked(AuthCache *cache, const char *username, const char *ip)
{
    if (!cache || !username || !ip)
        return 0;

    unsigned int index = hash_username_ip(username, ip);
    AuthNode *current = cache->buckets[index];

    while (current)
    {
        if (strcmp(current->username, username) == 0 && strcmp(current->ip_address, ip) == 0)
        {
            if (current->block_until_ip > time(NULL) || current->block_until_user > time(NULL))
            {
                return 1; // User is blocked
            }
            return 0; // Not blocked
        }
        current = current->next;
    }

    return 0; // User not found, so not blocked
}

void reset_auth_attempts(AuthCache *cache, const char *username, const char *ip)
{
    if (!cache || !username || !ip)
        return;

    unsigned int index = hash_username_ip(username, ip);
    AuthNode *current = cache->buckets[index];
    AuthNode *prev = NULL;

    while (current)
    {
        if (strcmp(current->username, username) == 0 && strcmp(current->ip_address, ip) == 0)
        {
            // Remove from the linked list
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                cache->buckets[index] = current->next;
            }
            free(current);
            return; // Successfully removed
        }
        prev = current;
        current = current->next;
    }
}

void destroy_auth_cache(AuthCache *cache) {
    if (!cache) return;

    // Loop through all buckets in the hash table
    for (int i = 0; i < AUTH_HASH_SIZE; i++) {
        AuthNode *current = cache->buckets[i];

        // Free each node in the linked list
        while (current) {
            AuthNode *next = current->next;
            free(current);
            current = next;
        }

        // Set the bucket to NULL after freeing all nodes
        cache->buckets[i] = NULL;
    }

    // Free the cache structure itself
    free(cache);
}