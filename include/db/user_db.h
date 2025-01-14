#ifndef USER_DB_H
#define USER_DB_H

#include <sqlite3.h>
#include <time.h>
#include <stdlib.h>
// Status codes for database operations
#define DB_SUCCESS          0
#define DB_ERROR          -1
#define DB_USER_EXISTS    -2
#define DB_AUTH_FAILED    -3
#define DB_NOT_FOUND     -4

// User data structure
typedef struct {
    int user_id;
    char username[32];
    char password_hash[64];    // Store hashed password, never plaintext
    time_t created_at;         // Account creation timestamp
    time_t last_login;         // Last successful login
    int login_count;           // Number of successful logins
} UserData;

// Database connection handle
typedef struct {
    sqlite3* db;                 // SQLite database connection
    sqlite3_stmt* auth_stmt;     // Prepared statement for authentication
    sqlite3_stmt* create_stmt;   // Prepared statement for user creation
    sqlite3_stmt* get_user_stmt; // For fetching user data
    sqlite3_stmt* update_user_stmt; // For updating user data
} UserDB;

// Database initialization and cleanup
UserDB* init_user_db(const char* db_path);
void close_user_db(UserDB* db);

// User management functions
int create_user(UserDB* db, const char* username, const char* password);
int authenticate_user(UserDB* db, const char* username, const char* password);
int get_user_data(UserDB* db, const char* username, UserData* data);
int update_last_login(UserDB* db, const char* username);

// Database maintenance functions
int init_db_tables(UserDB* db);
int backup_db(UserDB* db, const char* backup_path);

#endif /* USER_DB_H */