#ifndef _USERS_H
#define _USERS_H

#include <stdbool.h>


#define MAX_LEN_USER_FILE 	((int)(1024 * 1024 * 128)) //128 mB
#define MAX_USER_NAME_LEN 	((int) 64)
#define MAX_PWD_LEN 		((int) 64)
#define MAX_DETAILS_LEN		((int) 64)

/*
 *
 * Read users-info.txt into user_file_content array
 *
 * @return 	0 is success
			1 if file couldn't be opened
			2 error reading file
 */
int load_user_file();

void add_user_base_folder();

bool exist_username_with_password(const char* username_in, const char* password_in);

bool exist_username(const char* username_in);

char* get_users_details();

int add_user(const char* username, const char* pwd, const char* details);

#endif