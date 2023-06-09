#include "users.h"
#include "log.c/log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ftw.h>
#define _XOPEN_SOURCE 500


//Contains complete user file
char user_file_content[MAX_LEN_USER_FILE];

bool exist_username_with_password(const char* username_in, const char* password_in)
{
	//check if user folder exists
	char path[128] = "users/";
	
    strcat(path, username_in);
    strcat(path, "/");
    struct stat sb;

    if (!(stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))) { //folder does not exist
        return false;
    }

	if (password_in == NULL) //only check if user exists
	{
		return true;
	}

	//check if content of passwd.txt is like the password_in
	strcat(path, "passwd.txt");
	
	FILE *fp = fopen(path, "r");
	if (!fp)
	{
		log_error("Error opening passwd.txt file, but the user exists.");
		return false;
	}
	char password_read[MAX_PWD_LEN];
	for (int i=0; i < MAX_PWD_LEN; i++)
	{
		password_read[i] = 0;
	}
	int len = fread(password_read, MAX_PWD_LEN, 1, fp);
	if (len == 1 && ferror(fp))
	{
		log_error("Error reading passwd.txt file content");
		fclose(fp);
		return false;
	}
	log_trace("Read password: %s", password_read);
	if (!strcmp(password_read, password_in))
	{ //correct
		fclose(fp);
		return true;
	}

	log_warn("Wrong provided password");
	fclose(fp);
	return false;
}



bool exist_username(const char* username_in)
{
	return exist_username_with_password(username_in, NULL);
}

void add_user_base_folder()
{
	//adds users/ folder
	struct stat st = {0};

	if (stat("users", &st) == -1) {
		mkdir("users", 0700);
	}

	if (stat("default_data", &st) == -1) {
		log_warn("Default data could not be found, no bach linking :(");
	} else{
		log_trace("Linking bach.wav example file");
		link("default_data/bach.wav", "users/bach.wav");
	}
}

static int unlink_cb(const char *fpath, 
	__attribute__ ((unused)) const struct stat *sb, 
	__attribute__ ((unused)) int typeflag, 
	__attribute__ ((unused)) struct FTW *ftwbuf)
{
    int rv = remove(fpath);
    if (rv)
	{
        perror(fpath);
	}

    return rv;
}

static int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

void add_user_folder_and_password(const char* username, const char* password)
{
    //remove user folder for clean beginning
    char path[128] = "users/";
	
    strcat(path, username);
    strcat(path, "/");
    
	rmrf(path);

    //create new users folder
	struct stat st = {0};
	if (stat(path, &st) == -1) {
		mkdir(path, 0700);
	}

	//create password file and write it
	strcat(path, "passwd.txt");
	FILE *fp = fopen(path, "w");
	if (!fp)
	{
		return;
	}
	fwrite(password, strlen(password), 1, fp);
	fclose(fp);

}

/**
 * Register the given user.
 * Adds users info to users-info.txt, creates user folder
 *
 */
int add_user(const char* username, const char* pwd)
{	
    add_user_folder_and_password(username, pwd);

    return 0;
}