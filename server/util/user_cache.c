#include "util/user_cache.h"

UserCache* create_user_cache(void) {
    UserCache* cache = malloc(sizeof(UserCache));
    if (!cache) return NULL;

    memset(cache->buckets, 0, sizeof(cache->buckets));
    cache->size = 0;
    return cache;
}

int add_user(UserCache* cache, const char* username, int port, uint32_t session_key) {
    if (!cache || !username) return -1;
    
    // Check if user already exists
    if (has_user(cache, username)) return -1;
    
    unsigned int index = hash_username(username);
    
    // Create new node
    UserNode* node = malloc(sizeof(UserNode));
    if (!node) return -1;
    
    strncpy(node->username, username, MAX_USERNAME - 1);
    node->username[MAX_USERNAME - 1] = '\0';
    node->port = port;
    node->session_key = session_key;
    node->last_active = time(NULL);
    
    // Insert at head of bucket (O(1))
    node->next = cache->buckets[index];
    cache->buckets[index] = node;
    cache->size++;
    
    return 0;
}

int remove_user(UserCache* cache, const char* username) {
    if (!cache || !username) return -1;
    
    unsigned int index = hash_username(username);
    UserNode* current = cache->buckets[index];
    UserNode* prev = NULL;
    
    while (current) {
        if (strcmp(current->username, username) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                cache->buckets[index] = current->next;
            }
            free(current);
            cache->size--;
            return 0;
        }
        prev = current;
        current = current->next;
    }
    
    return -1;  // User not found
}

int get_user_port(UserCache* cache, const char* username) {
    if (!cache || !username) return -1;
    
    unsigned int index = hash_username(username);
    UserNode* current = cache->buckets[index];
    
    while (current) {
        if (strcmp(current->username, username) == 0) {
            return current->port;
        }
        current = current->next;
    }
    
    return -1;  // User not found
}

uint32_t get_user_session(UserCache* cache, const char* username) {
    if (!cache || !username) return 0;
    
    unsigned int index = hash_username(username);
    UserNode* current = cache->buckets[index];
    
    while (current) {
        if (strcmp(current->username, username) == 0) {
            return current->session_key;
        }
        current = current->next;
    }
    
    return 0;  // User not found
}

int has_user(UserCache* cache, const char* username) {
    return get_user_port(cache, username) != -1;
}

int is_port_in_use(UserCache* cache, int port) {
    if (!cache) return 0;
    
    for (int i = 0; i < HASH_SIZE; i++) {
        UserNode* current = cache->buckets[i];
        while (current) {
            if (current->port == port) return 1;
            current = current->next;
        }
    }
    
    return 0;
}

void cleanup_inactive_users(UserCache* cache, time_t timeout) {
    if (!cache) return;
    
    time_t current_time = time(NULL);
    
    for (int i = 0; i < HASH_SIZE; i++) {
        UserNode* current = cache->buckets[i];
        UserNode* prev = NULL;
        
        while (current) {
            if (current_time - current->last_active > timeout) {
                UserNode* to_delete = current;
                if (prev) {
                    prev->next = current->next;
                    current = current->next;
                } else {
                    cache->buckets[i] = current->next;
                    current = cache->buckets[i];
                }
                free(to_delete);
                cache->size--;
            } else {
                prev = current;
                current = current->next;
            }
        }
    }
}

void destroy_user_cache(UserCache* cache) {
    if (!cache) return;
    
    // Free all nodes in all buckets
    for (int i = 0; i < HASH_SIZE; i++) {
        UserNode* current = cache->buckets[i];
        while (current) {
            UserNode* next = current->next;
            free(current);
            current = next;
        }
    }
    
    free(cache);
}