#ifndef _USERS_H
#define _USERS_H

#include <stdbool.h>

#define MAX_USER_NAME_LEN 	((int) 64)
#define MAX_PWD_LEN 		((int) 64)
#define MAX_DETAILS_LEN		((int) 64)

void add_user_base_folder();

bool exist_username_with_password(const char* username_in, const char* password_in);

bool exist_username(const char* username_in);

int add_user(const char* username, const char* pwd);

/**
 * @brief Writes the given key for the given user to it's key file, the 'key.txt'.
 * If this file already exist, it will be overwritten, otherwise it will be newly created.
 * 
 * @return int 0 for success, otherwise != 0.
 */
int write_key(const char* user_name, const char* key);

int read_key(const char* user_name, char** key_back);

int delete_key(const char* user_name);


#endif