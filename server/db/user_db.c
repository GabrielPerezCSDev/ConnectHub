#include "db/user_db.h"
#include "db/db_config.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Simple password hashing (in practice, use a proper hashing library like bcrypt)
static void hash_password(const char* password, char* hash_out) {
    // TODO: Implement proper password hashing
    // This is just for development, NOT for production
    snprintf(hash_out, MAX_PASSWORD_LENGTH, "hashed_%s", password);
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
    const char* auth_sql = "SELECT user_id FROM users WHERE username = ? AND password_hash = ?";
    if (sqlite3_prepare_v2(db->db, auth_sql, -1, &db->auth_stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }

    // Add other prepared statements here...

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

    char password_hash[MAX_PASSWORD_LENGTH];
    hash_password(password, password_hash);

    sqlite3_reset(db->auth_stmt);
    sqlite3_bind_text(db->auth_stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(db->auth_stmt, 2, password_hash, -1, SQLITE_STATIC);

    int rc = sqlite3_step(db->auth_stmt);
    
    if (rc == SQLITE_ROW) {
        // Update last login time and count
        update_last_login(db, username);
        return DB_SUCCESS;
    }
    
    return DB_AUTH_FAILED;
}

int update_last_login(UserDB* db, const char* username) {
    const char* sql = "UPDATE users SET last_login = ?, login_count = login_count + 1 WHERE username = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return DB_ERROR;

    time_t now = time(NULL);
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? DB_SUCCESS : DB_ERROR;
}

void close_user_db(UserDB* db) {
    if (!db) return;

    if (db->auth_stmt) sqlite3_finalize(db->auth_stmt);
    if (db->create_stmt) sqlite3_finalize(db->create_stmt);
    if (db->update_stmt) sqlite3_finalize(db->update_stmt);
    if (db->fetch_stmt) sqlite3_finalize(db->fetch_stmt);
    
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