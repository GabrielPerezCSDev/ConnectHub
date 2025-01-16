#include "db/user_db.h"
#include "db/db_config.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <bcrypt/bcrypt.h> 

// Simple password hashing (in practice, use a proper hashing library like bcrypt)
static void hash_password(const char* password, char* hash_out) {
    if (!password || !hash_out) return;
    // Generate a salt and hash the password
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];
    
    // Generate a salt with work factor 12
    if (bcrypt_gensalt(12, salt) != 0) {
        // Handle salt generation error
        return;
    }
    
    // Hash the password
    if (bcrypt_hashpw(password, salt, hash) != 0) {
        // Handle hashing error
        return;
    }
    
    // Copy the hash to output
    strncpy(hash_out, hash, BCRYPT_HASHSIZE);
}

// For verifying passwords
static int verify_password(const char* password, const char* hash) {
    return (bcrypt_checkpw(password, hash) == 0);
}

UserDB* init_user_db(const char* db_path) {
    UserDB* db = malloc(sizeof(UserDB));
    if (!db) return NULL;

    // Open database connection
    int rc = sqlite3_open(db_path, &db->db);
    if (rc != SQLITE_OK) {
        printf("Cannot open database: %s\n", sqlite3_errmsg(db->db));
        free(db);
        return NULL;
    }

    // Initialize database tables
    if (init_db_tables(db) != DB_SUCCESS) {
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }
    // Prepare statements
    const char* auth_sql = "SELECT password_hash FROM users WHERE username = ?";
    const char* get_user_sql = "SELECT * FROM users WHERE username = ?";
    const char* update_user_sql = "UPDATE users SET last_login = ?, login_count = ? WHERE username = ?";
    
    if (sqlite3_prepare_v2(db->db, auth_sql, -1, &db->auth_stmt, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db->db, get_user_sql, -1, &db->get_user_stmt, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db->db, update_user_sql, -1, &db->update_user_stmt, NULL) != SQLITE_OK) {
        
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }

    return db;
}


int init_db_tables(UserDB* db) {
    char* err_msg = NULL;
    int rc = sqlite3_exec(db->db, CREATE_USERS_TABLE_SQL, NULL, NULL, &err_msg);
    
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return DB_ERROR;
    }

    rc = sqlite3_exec(db->db, CREATE_USERNAME_INDEX_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return DB_ERROR;
    }

    return DB_SUCCESS;
}

int create_user(UserDB* db, const char* username, const char* password) {
    if (!db || !username || !password) return DB_ERROR;
    
    char password_hash[MAX_PASSWORD_LENGTH];
    hash_password(password, password_hash);

    const char* sql = "INSERT INTO users (username, password_hash, created_at) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return DB_ERROR;

    time_t now = time(NULL);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, now);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_CONSTRAINT) {
        return DB_USER_EXISTS;
    }
    
    return (rc == SQLITE_DONE) ? DB_SUCCESS : DB_ERROR;
}

int authenticate_user(UserDB* db, const char* username, const char* password) {
    if (!db || !username || !password) return DB_ERROR;

    
    if (!db->auth_stmt) {
        return DB_ERROR;
    }

    sqlite3_reset(db->auth_stmt);
    
    int bind_result = sqlite3_bind_text(db->auth_stmt, 1, username, -1, SQLITE_STATIC);
    if (bind_result != SQLITE_OK) {
        return DB_ERROR;
    }

    int rc = sqlite3_step(db->auth_stmt);
    
    if (rc == SQLITE_ROW) {
        const char* stored_hash = (const char*)sqlite3_column_text(db->auth_stmt, 0);
        if (!stored_hash) {
            return DB_ERROR;
        }
        
        int verify_result = verify_password(password, stored_hash);
        
        if (verify_result) {
            update_last_login(db, username);
            return DB_SUCCESS;
        }
    }
    
    return DB_AUTH_FAILED;
}

int update_last_login(UserDB* db, const char* username) {
    if (!db || !username) {
        printf("[DEBUG] Error: Database or username is null.\n");
        return DB_ERROR;
    }
    
    if (!db->update_user_stmt) {
        printf("[DEBUG] Error: Prepared statement for updating user is null.\n");
        return DB_ERROR;
    }
    
    // Add this to see what SQL statement we're using
    
    sqlite3_reset(db->update_user_stmt);
    
    time_t now = time(NULL);
    if (now == -1) {
        printf("[DEBUG] Error: Failed to get current time.\n");
        return DB_ERROR;
    }
    
    int bind_time = sqlite3_bind_int64(db->update_user_stmt, 1, now);
    if (bind_time != SQLITE_OK) {
        return DB_ERROR;
    }
    
    int bind_user = sqlite3_bind_text(db->update_user_stmt, 2, username, -1, SQLITE_STATIC);
    if (bind_user != SQLITE_OK) {
        printf("[DEBUG] Error: Failed to bind username. SQLite error code: %d\n", bind_user);
        return DB_ERROR;
    }
    
    int rc = sqlite3_step(db->update_user_stmt);
    
    // Get number of rows changed
    int rows_changed = sqlite3_changes(db->db);
    
    // Reset statement after execution
    sqlite3_reset(db->update_user_stmt);
    
    if (rc == SQLITE_DONE && rows_changed > 0) {
        printf("[DEBUG] Update successful. User '%s' last login time updated.\n", username);
        return DB_SUCCESS;
    }
    
    printf("[DEBUG] Update failed. SQLite return code: %d\n", rc);
    return DB_ERROR;
}


void close_user_db(UserDB* db) {
    if (!db) return;

    if (db->auth_stmt) sqlite3_finalize(db->auth_stmt);
    if (db->update_user_stmt) sqlite3_finalize(db->update_user_stmt);
    if (db->get_user_stmt) sqlite3_finalize(db->get_user_stmt);
    
    if (db->db) sqlite3_close(db->db);
    free(db);
}

int backup_db(UserDB* db, const char* backup_path) {
    sqlite3* backup_db;
    int rc = sqlite3_open(backup_path, &backup_db);
    if (rc != SQLITE_OK) return DB_ERROR;

    sqlite3_backup* backup = sqlite3_backup_init(backup_db, "main", db->db, "main");
    if (backup) {
        sqlite3_backup_step(backup, -1);
        sqlite3_backup_finish(backup);
    }

    rc = sqlite3_errcode(backup_db);
    sqlite3_close(backup_db);

    return (rc == SQLITE_OK) ? DB_SUCCESS : DB_ERROR;
}