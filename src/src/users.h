#ifndef _USERS_H
#define _USERS_H

#include <stdbool.h>


#define MAX_LEN_USER_FILE 65536

/*
 *
 * Read users-info.txt into user_file_content array
 *
 * @return 	0 is success
			1 if file couldn't be opened
			2 error reading file
 */
int load_user_file();

bool exist_username_with_password(char* username_in, char* password_in);

bool exist_username(char* username_in);

int add_user(char* username, char* pwd, char* details);

#endif