#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "users.h"
#include "base64.h"
#include "granular.h"
#include "file_handler.h"
#include "log.c/log.h"

#define ARRSIZE(a) (sizeof(a)/sizeof(a[0]))


//Flag for debugging, forces the creation of a new clean setup when service is started
#define FORCE_NEW_SETUP false

char* current_user;
granular_info* last_granular_info;

char* ask(const char* prompt)
{
	printf("%s", prompt);

	static char buf[2048];
	char *tok;

	if (fgets(buf, ARRSIZE(buf), stdin)) {
		tok = strchr(buf, '\n');
		if (tok) *tok = '\0';
	} else {
		*buf = '\0';
	}

	return buf;
}

/**
 * Perform setup of service if the service does not exist yet.
 *
 * Creates users-info.txt
 * Deletes users/ directory if it exist
 * Creates users/ directory
 *
 */
void setup_service()
{
	add_user_base_folder();

	FILE* fp = fopen("users/users-info.txt", "r");
	
	if (fp != NULL && !FORCE_NEW_SETUP)
	{ //file already exist, do not perform setup
		fclose(fp);
		return;
	}
	//file doesn't exist, fclose is not necessary then
	
	//create empty file
	fp = fopen("users/users-info.txt", "w");
	fclose(fp);

	srand(time(NULL)); //create random seed

}


/**
 * Login prompt and checking.
 */
bool login()
{
	char *username, *password;

	char* username_tmp = ask("Username: ");
	username = strdup(username_tmp);

	char* password_tmp = ask("Password: ");
	password = strdup(password_tmp);
	
	if (exist_username_with_password(username, password))
	{
		printf("Welcome \'%s\'!\n", username);
		current_user = strdup(username);
		free(username);
		free(password);
		return true;
	}

	printf("Wrong password\n");
	free(username);
	free(password);
	
	return false;
}

void register_user()
{
	log_trace("Register call");
	char* username  = ask("Username: ");
	char* username_cpy = strdup(username);
	log_trace("Entered user_name: %s", username_cpy);
	char* password	= ask("Password: ");
	char* password_cpy = strdup(password);
	log_trace("Entered password: %s", password_cpy);
	char* details 	= ask("Please share some details about yourself: ");
	char* details_cpy = strdup(details);
	log_trace("Entered details: %s", details_cpy);

	//check if username does not exist
	int res = load_user_file(); //load current user file
	if (res)
	{
		log_trace("Couldnt read user_file, abort register_user");
		printf("Internal error, user couldn't created\n");
		return;
	}
	bool exist = exist_username(username_cpy);
	if (exist)
	{
		printf("user already exist!\n");
		log_trace("Couldnt create user '%s' - already exists", username_cpy);
	} else {
		add_user(username_cpy, password_cpy, details_cpy);
		printf("ok\n");
		log_trace("User '%s' created successfully", username_cpy);
	}
	free(username_cpy);
	free(password_cpy);
	free(details_cpy);

}

void granulize_call()
{
	printf("Enter a file name: ");
	char file_name[1024];
	fgets(file_name, 1024, stdin);

	char* tok = strchr(file_name, '\n'); //remove \n 
	if (tok) *tok = '\0';

	//check that file name is valid
	char *dot = strrchr(file_name, '.');
	if (!dot)
	{
		printf("File ending is missing\n");
	}
	if (strcmp(dot, ".wav") && strcmp(dot, ".pcm"))
	{
		printf("file has to end with .wav or .pcm\n");
		return;
	}
	
	//build path
	char file_name_complete[128];
	strcpy(file_name_complete, "users/");
	strcat(file_name_complete, current_user);
	strcat(file_name_complete, "/");
	strcat(file_name_complete, file_name);

	if (!strcmp(dot, ".wav"))
	{ //wave handler
		printf("Wav handler not yet implemented\n");
	} else if (!strcmp(dot, ".pcm"))
	{
		//printf("read .pcm file %s\n", file_name_complete);
		char *p_buf;
		
		int len = read_pcm(file_name_complete, &p_buf);
		//printf("read %s\n", p_buf);

		//handle data:
		char *new_sample;
		int new_sample_len;
		granular_info* info = granulize(p_buf, len, &new_sample, &new_sample_len);

		last_granular_info = info;

		//write data to users folder
		memset(file_name_complete, 0, 128);
		strcpy(file_name_complete, "users/");
		strcat(file_name_complete, current_user);
		strcat(file_name_complete, "/");
		strcat(file_name_complete, "granulized.pcm\0");
		//strcat(file_name_complete, file_name_orig);
		
		int res = write_pcm(file_name_complete, new_sample, new_sample_len);
		if (res)
		{
			//TODO proper error checking
		}
		printf("written to file %s\n", file_name_complete);
		
	}	
}

char* build_user_path(const char* file_name)
{
	static char string[1024];
	memset(string, 0, 1024);
	strcpy(string, "users/");
	strcat(string, current_user);
	strcat(string, "/");
	strcat(string, file_name);
	return string;
}

void upload_file(char* ending)
{
	char* file_name_in = ask("Enter file name for new file: ");
	if (!file_ends_with(file_name_in, ending))
	{
		printf("File has to end with %s\n", ending);
		return;
	}
	if (path_contains_illegal_chars(file_name_in))
	{
		printf("File name contains illegal characters\n");
		return;
	}

	printf("Enter base64 encoded wave file\n"); //, maximum size 1024 bytes:\n");
	char base64encoded[1024];
	fgets(base64encoded, 1024, stdin);

	char input[1024];
	//decode and write to file:
	int len = Base64decode(input, base64encoded);
	//printf("%s\n", input);

	if (len <= 0)
	{
		printf("Error parsing the b64\n");
		return;
	}
	
	//build complete filepath with name
	char file_name_complete[128];
	strcpy(file_name_complete, "users/");
	strcat(file_name_complete, current_user);
	strcat(file_name_complete, "/");
	strcat(file_name_complete, file_name_in);

	FILE* fp = fopen(file_name_complete, "w");
	if (!fp)
	{
		perror("fopen");
		return;
	}

	fwrite(input, 1, len, fp);
	fclose(fp);
	//TODO error checking
	printf("Success\n");
}

void upload_pcm_file_call()
{
	upload_file(".pcm\0");
}

void upload_wav_file_call()
{
	upload_file(".wav\0");
}

void download_wav_file_call()
{
	printf("TODO\n");
}

void download_pcm_file_call()
{
	char* file_name = ask("Filename: ");
	
	//sanitize
	if (path_contains_illegal_chars(file_name))
	{
		printf("Error - filename contains illegal character\n");
		return;
	}
	//check if filename ending is correct
	char *dot = strrchr(file_name, '.');
	if (!(dot && !strcmp(dot, ".pcm")))
	{
		printf("Error - filename does not end with .pcm\n");
		return;
	}
	
	//build path with filename
	char* path = build_user_path(file_name);
	printf("read file from path %s\n", path);
	char* path_cpy = strdup(path);
	

	//get file content
	char *p_buf;
	int len = read_pcm(path_cpy, &p_buf);
	//printf("File: %s\n Len: %i\n", p_buf, len);

	//b64 encode
	char encoded[20640];
	len = Base64encode(encoded, p_buf, len);
	printf("File: \n%s\n", encoded);

}

void granulize_info_call()
{
	if (last_granular_info)
	{
		print_granular_info(last_granular_info);
	} else {
		printf("No last granular infos to print\n");
	}
}

void account_call()
{
	
}

void help_call()
{
	printf("upload wav - uploads a .wav file into own profile, encoded as base64\n");
	printf("upload pcm - uploads a .pcm (pulse-code modulation) file into own profile, encoded as base64\n");
	printf("download wav - downloads a .wav file from own profile, encoded as base64\n");
	printf("download pcm - downloads a .pcm file from own profile, encoded as base64\n");
	printf("granulize - performs granulization algorithm with random parameters on .pcm or .wav file\n");
	printf("granulize info - more details about last granulization process\n");
	printf("account - show account details\n");
	printf("help - this prompt\n\n");
}

int main()
{	

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	printf("  _____ _____            _   _ _    _ _      _____ ____________ _____  \n");
	printf(" / ____|  __ \\     /\\   | \\ | | |  | | |    |_   _|___  /  ____|  __ \\ \n");
	printf("| |  __| |__) |   /  \\  |  \\| | |  | | |      | |    / /| |__  | |__) | \n");
	printf("| | |_ |  _  /   / /\\ \\ | . ` | |  | | |      | |   / / |  __| |  _  / \n");
	printf("| |__| | | \\ \\  / ____ \\| |\\  | |__| | |____ _| |_ / /__| |____| | \\ \\ \n");
	printf(" \\_____|_|  \\_\\/_/    \\_\\_| \\_|\\____/|______|_____/_____|______|_|  \\_\\ \n\n");
	
	setup_service();

	while (1)
	{
		char* in = ask("Hello! Do you want to login (l) or register (r)?\n >\0");
		if (!strcmp(in, "register") || !strcmp(in, "r"))
		{
			register_user();
		} else if (!strcmp(in, "login") || !strcmp(in, "l"))
		{
			bool worked = login();
			if (worked)
			{
				break; //enter main loop
			}
		} else {
			printf("Please enter login or register\n");
		}
	}

	

	struct {
		const char *name;
		void (*func)();
	} cmds[] = {
		{ "upload wav\n", upload_wav_file_call },
		{ "upload pcm\n", upload_pcm_file_call },
		{ "download wav\n", download_wav_file_call },
		{ "download pcm\n", download_pcm_file_call },
		{ "granulize info\n", granulize_info_call },
		{ "granulize\n", granulize_call },
		{ "account\n", account_call },
		{ "help\n", help_call }
	};

	char cmd[32];
	while (1)
	{
		printf("What do you wanna do?\n > ");
		fgets(cmd, 32, stdin);

		int i;
		for (i = 0; i < (int) ARRSIZE(cmds); i++) {
			if (!strcmp(cmd, cmds[i].name)) {
				cmds[i].func();
				break;
			}
		}

		if (i == ARRSIZE(cmds))
		{
			printf("Unknown command: %s Enter help for helping prompt\n", cmd);
		}
	}
}
