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
 * When succesful login, current_user will be placed with username.
 */
bool login()
{
	char *username, *password;
	log_trace("Login call");

	char* username_tmp = ask("Username: ");
	username = strdup(username_tmp);
	log_trace("Entered user_name: %s", username);

	char* password_tmp = ask("Password: ");
	password = strdup(password_tmp);
	log_trace("Entered password: %s", password);

	if (exist_username_with_password(username, password))
	{
		printf("Welcome \'%s\'!\n", username);
		log_trace("User '%s' successful login", username);
		current_user = strdup(username);
		free(username);
		free(password);
		return true;
	}

	printf("Wrong password\n");
	log_trace("User '%s' provided wrong credentials: %s", username, password);
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
	enum FILE_MODE {
		PCM,
		WAV
	};

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
	log_debug("Correct file ending: %s", dot);
	
	//build path
	char file_name_complete[128];
	strcpy(file_name_complete, "users/");
	strcat(file_name_complete, current_user);
	strcat(file_name_complete, "/");
	strcat(file_name_complete, file_name);
	log_debug("Complete file path: %s", file_name_complete);

	int len;
	char* p_data;
	WavHeader* w_header;
	enum FILE_MODE file_mode;

	if (!strcmp(dot, ".wav"))
	{
		file_mode = WAV;
		len = read_wav(file_name_complete, &p_data, &w_header);
		if (len < 1)
		{
			log_error("No data from read_wav, abort granulization");
			printf("Error reading file\n");
			return;
		}
	} else if (!strcmp(dot, ".pcm"))
	{
		file_mode = PCM;
		len = read_pcm(file_name_complete, &p_data);
		if (len < 1)
		{
			log_error("No data from read_wav, abort granulization");
			printf("Error reading file\n");
			return;
		}
	} else {
		printf("Error, wrong file format");
		log_error("no pcm or wav file for input, instead: %s", dot);
	}
	log_info("Successfully read data");

	//handle data:
	char *new_sample;
	int new_sample_len;
	granular_info* info = granulize(p_data, len, &new_sample, &new_sample_len);

	last_granular_info = info;

	//write data to users folder
	memset(file_name_complete, 0, 128);
	strcpy(file_name_complete, "users/");
	strcat(file_name_complete, current_user);
	strcat(file_name_complete, "/");
	
	
	//write file
	int res;
	if (file_mode == WAV)
	{
		strcat(file_name_complete, "granulized.wav\0");
		res = write_wav(file_name_complete, new_sample, w_header, new_sample_len);
	} else if (file_mode == PCM)
	{
		strcat(file_name_complete, "granulized.pcm\0");
		res = write_pcm(file_name_complete, new_sample, new_sample_len);
	} else {
		printf("Error, wrong file format");
		log_error("no pcm or wav file for input, instead: %s", dot);
	}

	if (res)
	{
		//TODO proper error checking
	}
	printf("written to file %s\n", file_name_complete);
	log_info("written to file %s", file_name_complete);
}

static char* build_user_path(const char* file_name)
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
	log_trace("Upload file %s call", ending);

	char* file_name_in = ask("Enter file name for new file: ");
	if (!file_ends_with(file_name_in, ending))
	{
		log_trace("File call cancelled: wrong file ending '%s'", ending);
		printf("File has to end with %s\n", ending);
		return;
	}
	if (path_contains_illegal_chars(file_name_in))
	{
		log_trace("File call cancelled: file contains illegal chars '%s'", file_name_in);
		printf("File name contains illegal characters\n");
		return;
	}

	printf("Enter base64 encoded wave file\n"); //, maximum size 1024 bytes:\n");
	char base64encoded[1024];
	fgets(base64encoded, 1024, stdin);

	char input[1024];
	//decode and write to file:
	int len = Base64decode(input, base64encoded);

	if (len <= 0)
	{
		log_trace("Error parsing the b64: %s", base64encoded);
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
		printf("Error opening file\n");
		log_warn("Error opening file");
		return;
	}

	int written = fwrite(input, 1, len, fp);
	if (written != len)
	{
		printf("Error writing to file\n");
		log_warn("Error writing to file");
		return;
	}
	fclose(fp); //TODO error handling here
	log_trace("Success uploaded file to %s", file_name_complete);

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


/**
 * @return entered sanitized filename, or NULL if error occurred
 * Example call: ask_correct_filename(".wav")
 */
static char* ask_correct_filename(const char* file_ending)
{
	log_trace("Download %s file call", file_ending);
	char* file_name = ask("Filename: ");

	//sanitize
	if (path_contains_illegal_chars(file_name))
	{
		log_trace("File call cancelled: file contains illegal chars '%s'", file_name);
		printf("Error - filename contains illegal character\n");
		return NULL;
	}
	//check if filename ending is correct
	char *dot = strrchr(file_name, '.');
	if (!(dot && !strcmp(dot, file_ending)))
	{
		log_trace("Download pcm aborted, filename does not end with %s: '%s'", file_ending, file_name);
		printf("Error - filename does not end with %s\n", file_ending);
		return NULL;
	}
	
	log_trace("Valid filename");
	
	return file_name;
}

void download_file_call(const char* ending)
{
	log_trace("Download file call");
	char* file_name = ask_correct_filename(ending);
	if (!file_name)
	{
		return;
	}
	
	//build path with filename
	char* path = build_user_path(file_name);
	printf("read file from path %s\n", path);
	char* path_cpy = strdup(path);
	
	//get file content
	char *p_buf;
	int len = read_pcm(path_cpy, &p_buf);
	
	//b64 encode
	char encoded[20640];
	len = Base64encode(encoded, p_buf, len);
	printf("File: \n%s\n", encoded);
	log_trace("Successfully sent file");
}

void download_wav_file_call()
{
	download_file_call(".wav");
}

void download_pcm_file_call()
{
	download_file_call(".pcm");
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
		printf("What do you want to do?\n > ");
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
