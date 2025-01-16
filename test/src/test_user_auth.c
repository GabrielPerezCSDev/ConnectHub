#include "unity/unity.h"
#include "db/user_db.h"

static UserDB* setup_test_db(void) {
    UserDB* db = malloc(sizeof(UserDB));
    if (!db) return NULL;

    // Open database connection
    int rc = sqlite3_open(":memory:", &db->db);
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

    // Prepare statements (matching your implementation)
    const char* auth_sql = "SELECT password_hash FROM users WHERE username = ?";
    const char* get_user_sql = "SELECT * FROM users WHERE username = ?";
    const char* update_user_sql = "UPDATE users SET last_login = ?, login_count = login_count + 1 WHERE username = ?";
    
    if (sqlite3_prepare_v2(db->db, auth_sql, -1, &db->auth_stmt, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db->db, get_user_sql, -1, &db->get_user_stmt, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db->db, update_user_sql, -1, &db->update_user_stmt, NULL) != SQLITE_OK) {
        
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }

    return db;
}

void test_user_registration_success(void) {
    UserDB* db = setup_test_db();
    TEST_ASSERT_NOT_NULL(db);
    
    int result = create_user(db, "testuser", "testpass");
    TEST_ASSERT_EQUAL(DB_SUCCESS, result);
    
    close_user_db(db);
}

void test_user_registration_duplicate(void) {
    UserDB* db = setup_test_db();
    TEST_ASSERT_NOT_NULL(db);
    
    create_user(db, "testuser", "testpass");
    int result = create_user(db, "testuser", "testpass");
    TEST_ASSERT_EQUAL(DB_USER_EXISTS, result);
    
    close_user_db(db);
}

void test_user_authentication_success(void) {
    UserDB* db = setup_test_db();
    TEST_ASSERT_NOT_NULL(db);
   
    int create_result = create_user(db, "testuser", "testpass");
    TEST_ASSERT_EQUAL(DB_SUCCESS, create_result);
    
    // Verify the hash was stored
    sqlite3_reset(db->auth_stmt);
    sqlite3_bind_text(db->auth_stmt, 1, "testuser", -1, SQLITE_STATIC);
    int rc = sqlite3_step(db->auth_stmt);
    TEST_ASSERT_EQUAL(SQLITE_ROW, rc);
    
    int auth_result = authenticate_user(db, "testuser", "testpass");
    TEST_ASSERT_EQUAL(DB_SUCCESS, auth_result);
    close_user_db(db);
}

void test_user_authentication_failure(void) {
    UserDB* db = setup_test_db();
    TEST_ASSERT_NOT_NULL(db);
    
    create_user(db, "testuser", "testpass");
    int result = authenticate_user(db, "testuser", "wrongpass");
    TEST_ASSERT_EQUAL(DB_AUTH_FAILED, result);
    
    close_user_db(db);
}


void test_user_creation_null_params(void) {
    UserDB* db = setup_test_db();
    TEST_ASSERT_NOT_NULL(db);
    
    TEST_ASSERT_EQUAL(DB_ERROR, create_user(db, NULL, "pass"));
    TEST_ASSERT_EQUAL(DB_ERROR, create_user(db, "user", NULL));
    TEST_ASSERT_EQUAL(DB_ERROR, create_user(NULL, "user", "pass"));
    
    close_user_db(db);
}


void test_update_last_login(void) {
    UserDB* db = setup_test_db();
    TEST_ASSERT_NOT_NULL(db);
    
    // Create and verify user first
    int create_result = create_user(db, "testuser", "testpass");
    TEST_ASSERT_EQUAL(DB_SUCCESS, create_result);
    
    // Verify user exists using get_user_stmt
    sqlite3_reset(db->get_user_stmt);
    sqlite3_bind_text(db->get_user_stmt, 1, "testuser", -1, SQLITE_STATIC);
    int rc = sqlite3_step(db->get_user_stmt);
    TEST_ASSERT_EQUAL(SQLITE_ROW, rc);  // Should find one row
    
    int update_result = update_last_login(db, "testuser");
    TEST_ASSERT_EQUAL(DB_SUCCESS, update_result);
    
    // Test non-existent user
    TEST_ASSERT_EQUAL(DB_ERROR, update_last_login(db, "nonexistent"));
    
    close_user_db(db);
}

void test_database_backup(void) {
    UserDB* db = setup_test_db();
    TEST_ASSERT_NOT_NULL(db);
    
    create_user(db, "testuser", "testpass");
    
    TEST_ASSERT_EQUAL(DB_SUCCESS, backup_db(db, ":memory:"));  // Test backup to memory
    TEST_ASSERT_NOT_EQUAL(DB_SUCCESS, backup_db(db, "/invalid/path/db.sqlite"));  // Test invalid path
    
    close_user_db(db);
}