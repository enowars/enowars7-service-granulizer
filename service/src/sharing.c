#include "sharing.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "users.h"
#include "file_handler.h"
#include "sha256/sha256.h"
#include "log.c/log.h"

long long get_time_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

char* generate_key(const char* username)
{
    unsigned long timestamp = get_time_milliseconds();

    char value_str[256];
    for (int i=0; i < 256; i++)
    {
        value_str[i] = 0;
    }
    snprintf(value_str, 256, "%ld", timestamp);
    
    for (size_t i=0; i < strlen(username); i++)
    { 
        //append username after timestamp in string, which length is sizeof(unsigned long) for its datatype
        value_str[i + sizeof(timestamp)] = username[i];
    }
    value_str[strlen(username) + sizeof(timestamp)] = 0; //append null byte

    uint8_t hash[SHA256_SIZE_BYTES];
    sha256(value_str, strlen(value_str), hash);
    
    char hash_str[SHA256_SIZE_BYTES * 2 + 1];
    char * p = hash_str;
    for (size_t i = 0; i < SHA256_SIZE_BYTES; i++) {
        p += sprintf(p, "%.2x", hash[i]);
    }
    
    char *back = strdup(hash_str);
    return back;
}

void sharing_allow_call(const char* username)
{
    //generate key and write it to own key.txt file
    char* key = generate_key(username);
    if (!key)
    {
        printf("Error generating key\n");
        return;
    }

    bool worked = write_key(username, key);
    if (!worked)
    {
        printf("Error writing key\n");
        free(key);
        return;
    }
    log_info("Sharing allowed, key: %s", key);
    //'publish' key
    printf("Sharing key: %s\n", key);

    free(key);
}

void sharing_disallow_call(const char* username)
{
    log_trace("Sharing disallow call");
    bool res = delete_key(username);
    if (!res)
    {
        printf("Error deleting key\n");
    } else {
        log_info("Sharing disallow success");
        printf("Key deleted successfully\n");
    }
}

//checks if a key file for this exists
bool sharing_is_allowed(const char* username)
{
    char path[128] = "users/";
    strcat(path, username);
    strcat(path, "/");
	strcat(path, "key.txt");

	return (access(path, F_OK) == 0); //check if the file exist
}

void sharing_use_key_call(const char* own_username, const char* username, const char* entered_key, const char* filename,
    granular_info** current_granular_info)
{
    char *key;
    bool res = read_key(username, &key);
    if (!res)
    {
        printf("Error reading key\n");
        return;
    }
    if (strncmp(entered_key, key, 2*SHA256_SIZE_BYTES) != 0)
    {
        printf("Wrong key provided\n");
        free(key);
        return;
    }
    free(key);
    log_info("User %s accesses shared files of user %s of file %s", own_username, username, filename);
	//build path
	char file_path[128];
    memset(file_path, 0, 128);

	//special case, TODO remove later and properly insert example files	
    strcpy(file_path, "users/");
    strcat(file_path, username);
    strcat(file_path, "/");
    strcat(file_path, filename);
    
    //granulize this file
    granulize_file(file_path, own_username, current_granular_info);
}