// include/db/db_config.h

#ifndef DB_CONFIG_H
#define DB_CONFIG_H

// Database file location
#define DEFAULT_DB_PATH "./data/user.db"

// Table creation SQL
#define CREATE_USERS_TABLE_SQL \
    "CREATE TABLE IF NOT EXISTS users (" \
    "user_id INTEGER PRIMARY KEY AUTOINCREMENT," \
    "username TEXT UNIQUE NOT NULL," \
    "password_hash TEXT NOT NULL," \
    "created_at INTEGER NOT NULL," \
    "last_login INTEGER," \
    "login_count INTEGER DEFAULT 0" \
    ");"

// Index creation SQL
#define CREATE_USERNAME_INDEX_SQL \
    "CREATE UNIQUE INDEX IF NOT EXISTS idx_username ON users(username);"

// Maximum lengths
#define MAX_USERNAME_LENGTH 32
#define MAX_PASSWORD_LENGTH 64

// Query timeouts
#define DB_TIMEOUT_MS 5000

#endif /* DB_CONFIG_H */