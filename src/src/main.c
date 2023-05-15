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

#define ARRSIZE(a) (sizeof(a)/sizeof(a[0]))

#define WAVE_HEADER_OFFSET 0x44

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

void init()
{
	srand(time(NULL)); //create random seed
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
	FILE* fp = fopen("users-info.txt", "r");
	
	if (fp != NULL && !FORCE_NEW_SETUP)
	{ //file already exist, do not perform setup
		fclose(fp);
		return;
	}
	//file doesn't exist, fclose is not necessary then
	
	//create empty file
	fp = fopen("users-info.txt", "w");
	fclose(fp);

	//delete users/ directory if it exist
	system("rm -rf users/");

	//create users/ directory
	int res = mkdir("users", 0777);

}

void login()
{
	char *username, *password;

	char* username_tmp = ask("Username: ");
	username = strdup(username_tmp);

	char* password_tmp = ask("Password: ");
	password = strdup(password_tmp);
	
	printf("Checking %s with %s\n", username, password);

	load_user_file(); //load current user file
	if (exist_username_with_password(username, password))
	{
		printf("Welcome %s!\n", username);
		current_user = username;
	} else {
		printf("Wrong password\n");
		exit(0);
	}
}

void reg()
{
	char* username  = ask("Username: ");
	char* password	= ask("Password: ");
	char* details 	= ask("Please share some details about yourself (will be privately stored in your account): ");

	//check if username does not exist
	load_user_file(); //load current user file
	bool exist = exist_username(username);
	if (exist)
	{
		printf("user already exist!\n");
	} else {
		add_user(username, password, details);
		printf("ok\n");
	}
}


void synth_file_call()
{
	printf("Enter a file name: ");
	char file_name[1024];
	fgets(file_name, 1024, stdin);

	char* tok = strchr(file_name, '\n'); //remove \n 
	if (tok) *tok = '\0';

	char* file_name_orig = strdup(file_name);

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
		printf("read .pcm file\n");
		char p_buf, *new_sample;
		int new_sample_len;
		int len = read_pcm(file_name_complete, &p_buf);
		printf("read %s\n", &p_buf);

		//handle data:
		granular_info* info = granulize(&p_buf, len, &new_sample, &new_sample_len);

		last_granular_info = info;

		//write data to users folder
		memset(file_name_complete, 0, 128);
		strcpy(file_name_complete, "users/");
		strcat(file_name_complete, current_user);
		strcat(file_name_complete, "/");
		strcat(file_name_complete, "granulized.");
		strcat(file_name_complete, file_name_orig);
		strcat(file_name_complete, "\0");
		printf("write to file %s\n", file_name_complete);
		int res = write_pcm(file_name_complete, new_sample, new_sample_len);

		//int res = write_pcm("output.pcm", new_sample, new_sample_len);
	}



	
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

	printf("Enter base64 encoded wave file, maximum size 1024 bytes:\n");
	char base64encoded[1024];
	fgets(base64encoded, 1024, stdin);

	char input[1024];
	//decode and write to file:
	int len = Base64decode(input, base64encoded);
	printf("%s\n", input);

	if (len <= 0)
	{
		printf("Error parsing the b64\n");
		return 1;
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
}

void upload_pcm_file_call()
{
	upload_file(".pcm\0");
}

void upload_wav_file_call()
{
	upload_file(".wav\0");
}







int main()
{
	char *ptr;
	int len = read_wav("example_saw.wav", &ptr);
	
	char p_buf, *new_sample;
	int out_len;

	//write wav back:
	//granulize(ptr, len, &new_sample, &out_len);

	write_wav("saw_out.wav", new_sample, out_len);

	//current_user = "a\0";
	//synth_file_call();
	exit(0);

	setup_service();

	
	char* in = ask("do you want to login (l) or register (r)?\n >");
	if (!strcmp(in, "register") || !strcmp(in, "r"))
	{
		reg();
	} else if (!strcmp(in, "login") || !strcmp(in, "l"))
	{
		login();
	} else {
		printf("Please enter login or register\n");
		exit(0);
	}

	init();

	struct {
		const char *name;
		void (*func)();
	} cmds[] = {
		{ "login\n", login },
		{ "register\n", reg },
		{ "upload wav\n", upload_wav_file_call },
		{ "upload pcm\n", upload_pcm_file_call },
		{ "granulize\n", synth_file_call }
		/*{ "users", api_list_users },
		{ "info", api_user_info },
		{ "login", api_login },
		{ "post", api_create_post },
		{ "posts", api_list_posts },
		{ "help", api_help },
		*/
	};

	printf("What do you wanna do?\n > ");
	
	char cmd[32];
	
	fgets(cmd, 32, stdin);


	int i;
	for (i = 0; i < ARRSIZE(cmds); i++) {
		if (!strcmp(cmd, cmds[i].name)) {
			cmds[i].func();
			break;
		}
	}

	if (i == ARRSIZE(cmds))
		printf("Unknown command: %s\n", cmd);

}
