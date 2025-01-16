// tests/include/test_user_auth.h
#ifndef TEST_USER_AUTH_H
#define TEST_USER_AUTH_H

void test_user_registration_success(void);
void test_user_registration_duplicate(void);
void test_user_authentication_success(void);
void test_user_authentication_failure(void);
void test_user_creation_null_params(void);
void test_update_last_login(void);
void test_database_backup(void);

#endif /* TEST_USER_AUTH_H */