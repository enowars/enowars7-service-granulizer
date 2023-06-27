#ifndef _USERS_H
#define _USERS_H

#include <stdbool.h>

#define MAX_USER_NAME_LEN 	((int) 64)
#define MAX_PWD_LEN 		((int) 64)
#define MAX_DETAILS_LEN		((int) 64)

void add_user_base_folder();

bool exist_username_with_password(const char* username_in, const char* password_in);

bool exist_username(const char* username_in);

bool add_user_folder_and_password(const char* username, const char* password);

/**
 * @brief Writes the given key for the given user to it's key file, the 'key.txt'.
 * If this file already exist, it will be overwritten, otherwise it will be newly created.
 * 
 * @return true for success, otherwise false
 */
bool write_key(const char* user_name, const char* key);

bool read_key(const char* user_name, char** key_back);

bool delete_key(const char* user_name);


#endif