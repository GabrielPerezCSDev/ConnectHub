-- Create users table
CREATE TABLE IF NOT EXISTS users (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    created_at INTEGER NOT NULL,    -- Unix timestamp
    last_login INTEGER,            -- Unix timestamp
    login_count INTEGER DEFAULT 0,
    
    -- Add any game-specific user data fields that need to persist
    rank INTEGER DEFAULT 0,
    experience INTEGER DEFAULT 0,
    coins INTEGER DEFAULT 0
);

-- Create indices for performance
CREATE UNIQUE INDEX IF NOT EXISTS idx_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_rank ON users(rank);

-- Insert some test data (remove in production)
INSERT OR IGNORE INTO users (username, password_hash, created_at, rank)
VALUES 
    ('test_user1', 'hashed_password123', strftime('%s', 'now'), 1),
    ('test_user2', 'hashed_password456', strftime('%s', 'now'), 2);