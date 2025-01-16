#include "unity/unity.h"
#include "test_user_auth.h"
// Add other test headers as we create them

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_WHITE   "\x1b[37m"

void setUp(void) {
    // Set up code that runs before each test
}

void tearDown(void) {
    // Clean up code that runs after each test
}

void run_auth_tests(void) {
    // Auth & User Management Tests
    printf(COLOR_YELLOW "\n============= Auth & User Management Tests =============\n" COLOR_RESET);

    // Registration Tests (Green)
    printf(COLOR_GREEN "\n--- Registration Tests ---\n" COLOR_RESET);
    RUN_TEST(test_user_registration_success);
    RUN_TEST(test_user_registration_duplicate);
    RUN_TEST(test_user_creation_null_params);
   

    // Authentication Tests (Yellow)
    printf(COLOR_GREEN  "\n--- Authentication Tests ---\n" COLOR_RESET);
    RUN_TEST(test_user_authentication_success);
    RUN_TEST(test_user_authentication_failure);
    

    // User Management Tests (Magenta)
    printf(COLOR_GREEN  "\n--- User Management Tests ---\n" COLOR_RESET);
    
    RUN_TEST(test_update_last_login);
    

    // Database Management Tests (Cyan)
    printf(COLOR_GREEN  "\n--- Database Management Tests ---\n" COLOR_RESET);
    RUN_TEST(test_database_backup);
    
}

int main(void) {
    UNITY_BEGIN();
    
     // user_auth tests
     run_auth_tests();
    
    return UNITY_END();
}