#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "users.h"
#include "b64.c/b64.h"
#include "granular.h"
#include "file_handler.h"
#include "sharing.h"
#include "sha256/sha256.h"
#include "log.c/log.h"

#define ARRSIZE(a) (sizeof(a)/sizeof(a[0]))

//With .pcm / .wav ending, therefore a filename needs to have a minimum length of 5
#define MIN_FILENAME_LEN ((int) 5)
#define MAX_FILENAME_LEN ((int) 64 + 5)

#define MAX_FILE_UPLOAD_LEN ((int) 1024 * 1024 * 5)
//5Mb of maximum file size for downloading
#define MAX_FILE_DOWNLOAD_LEN ((int) 1024 * 1024 * 5)

//Flag for debugging, forces the creation of a new clean setup when service is started
#define FORCE_NEW_SETUP false

char* current_user = NULL;
granular_info* last_granular_info = NULL;

static int base64encode_len(int len)
{
    return ((len + 2) / 3 * 4) + 1;
}

static void quit_call()
{
	log_info("Quitting");
	//frees all used memory
	if (current_user)
	{
		free(current_user);
		current_user = NULL;
	}
	destroy_granular_info(last_granular_info);
	
	
	printf("Byeee\n");
	exit(0);
}

char* ask(const char* prompt)
{
	printf("%s", prompt);

	static char buf[2048];
	char *tok;

	if (fgets(buf, ARRSIZE(buf), stdin)) {
		tok = strchr(buf, '\n');
		if (tok) *tok = '\0';
	} else { //In this case an EOF was detected, exit this program
		quit_call();
	}
	return buf;
}

bool containsIllegalChars(const char* input) {
    for (unsigned int i = 0; i < strlen(input); i++)
	{
		if ((input[i] < 'a' || input[i] > 'z') && (input[i] < 'A' || input[i] > 'Z') && (input[i] < '0' || input[i] > '9'))
		{
            return true;
        }
    }
    return false;
}


/**
 * Perform setup of service if the service does not exist yet.
 *
 * Deletes users/ directory if it exist
 * Creates users/ directory
 *
 */
void setup_service()
{
	//TODO comment in again
	srand(time(NULL)); //create random seed

	add_user_base_folder();

	log_info("Unknown user connected");
}

/**
 * Login prompt and checking.
 * When succesful login, current_user will be placed with username.
 */
bool login_user()
{
	char *username_cpy, *password_cpy;
	log_trace("Login call");

	char *username_tmp = ask("Username: ");
	username_cpy = strdup(username_tmp);
	log_trace("Entered user_name: %s", username_cpy);
	if (!strcmp(username_cpy, ""))
	{
		log_warn("Entered username is empty, abort");
		printf("Empty username is not allowed\n");
		free(username_cpy);
		return false;
	}
	if (containsIllegalChars(username_cpy))
	{
		log_warn("Username contains illegal chars, abort");
		printf("Username contains illegal characters. Allowed characters are only a-z, A-Z and 0-9\n");
		free(username_cpy);
		return false;
	}

	char *password_tmp = ask("Password: ");
	password_cpy = strdup(password_tmp);
	log_trace("Entered password: %s", password_cpy);
	if (!strcmp(password_cpy, ""))
	{
		log_warn("Entered password is empty, abort");
		printf("Empty password is not allowed\n");
		free(username_cpy);
		free(password_cpy);
		return false;
	}
	if (containsIllegalChars(password_cpy))
	{
		log_warn("Password contains illegal chars, abort");
		free(username_cpy);
		free(password_cpy);
		return false;
	}

	if (exist_username_with_password(username_cpy, password_cpy))
	{
		printf("Welcome \'%s\'!\n", username_cpy);
		log_trace("User '%s' successful login", username_cpy);
		current_user = strdup(username_cpy);
		set_current_user(current_user);
		free(username_cpy);
		free(password_cpy);
		return true;
	}

	printf("Wrong password\n");
	log_trace("User '%s' provided wrong credentials: %s", username_cpy, password_cpy);
	free(username_cpy);
	free(password_cpy);
	return false;
}

void register_user()
{
	log_trace("Register call");
	char* username  	= ask("Username: ");
	char* username_cpy 	= strdup(username);
	log_trace("Entered user_name: %s", username_cpy);
	if (!strcmp(username_cpy, ""))
	{
		log_warn("Entered username is empty, abort");
		printf("Empty username is not allowed\n");
		free(username_cpy);
		return;
	}
	if (strlen(username_cpy) > MAX_USER_NAME_LEN)
	{
		log_warn("Username too long, abort");
		printf("Username is too long, maximum allowed length is %i.\n", MAX_USER_NAME_LEN);
		free(username_cpy);
		return;
	}
	if (containsIllegalChars(username_cpy))
	{
		log_warn("Username contains illegal chars, abort");
		printf("Username contains illegal characters. Allowed characters are only a-z, A-Z and 0-9\n");
		free(username_cpy);
		return;
	}

	char* password		= ask("Password: ");
	char* password_cpy 	= strdup(password);
	log_trace("Entered password: %s", password_cpy);
	if (!strcmp(password_cpy, ""))
	{
		log_warn("Entered password is empty, abort");
		printf("Empty password is not allowed\n");
		free(username_cpy);
		free(password_cpy);
		return;
	}
	if (strlen(password_cpy) > MAX_PWD_LEN)
	{
		log_warn("Password too long, abort");
		printf("Password is too long, maximum allowed length is %i.\n", MAX_PWD_LEN);
		free(username_cpy);
		free(password_cpy);
		return;
	}
	if (containsIllegalChars(password_cpy))
	{
		log_warn("Password contains illegal chars, abort");
		printf("Password contains illegal characters. Allowed characters are only a-z, A-Z and 0-9\n");
		free(username_cpy);
		free(password_cpy);
		return;
	}

	bool exist = exist_username(username_cpy);
	if (exist)
	{
		printf("user already exist!\n");
		log_warn("Couldnt create user '%s' - already exists", username_cpy);
	} else {
		bool worked = add_user_folder_and_password(username_cpy, password_cpy);
		if (worked)
		{
			printf("ok\n");
			log_trace("User '%s' created successfully", username_cpy);
		} else {
			printf("error creating user\n");
			log_warn("User '%s' couldn't be created", username_cpy);
		}	
	}
	free(username_cpy);
	free(password_cpy);

}

void granulize_call()
{
	log_trace("Granulize call");

	printf("Enter a file name: ");
	char file_name[MAX_FILENAME_LEN];
	char *status = fgets(file_name, MAX_FILENAME_LEN, stdin);
	if (!status)
	{ //In this case an EOF was detected, then exit the program
		quit_call();
	}
	char *tok = strchr(file_name, '\n'); //remove \n 
	if (tok) *tok = '\0';
	
	//build path
	char file_name_complete[128];
	//special case, TODO remove later and properly insert example files
	if (!strcmp(file_name, "bach.wav"))
	{
		strcpy(file_name_complete, "default_data/bach.wav");
	} else {
		strcpy(file_name_complete, "users/");
		strcat(file_name_complete, current_user);
		strcat(file_name_complete, "/");
		strcat(file_name_complete, file_name);
	}
	log_debug("Complete file path: %s", file_name_complete);

	granulize_file(file_name_complete, current_user, &last_granular_info);
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

void upload_file(const char* ending)
{
	char* file_name_in = ask("Enter file name for new file: ");
	if (!file_ends_with(file_name_in, ending))
	{
		log_warn("File call cancelled: wrong file ending '%s', is '%s'", ending, file_name_in);
		printf("File has to end with %s\n", ending);
		return;
	}
	if (strlen(file_name_in) < MIN_FILENAME_LEN || strlen(file_name_in) > MAX_FILENAME_LEN)
	{
		log_warn("File upload cancelled, filename has invalid length.");
		printf("File upload cancelled, filename has invalid length. Only between %i - %i is allowed.\n", MIN_FILENAME_LEN, MAX_FILENAME_LEN);
		return;
	}
	if (path_contains_illegal_chars(file_name_in))
	{
		log_warn("File call cancelled: file contains illegal chars '%s'", file_name_in);
		printf("File name contains illegal characters\n");
		return;
	}

	printf("Enter base64 encoded wave file (maximum 4kB bytes long)\n");
	char *base64encoded = calloc(MAX_FILE_UPLOAD_LEN + 1, sizeof(char));
	char *res = fgets(base64encoded, MAX_FILE_UPLOAD_LEN, stdin); //TODO allocates much memory, refactor
	if (res == NULL)
	{
		printf("Error reading base64 data\n");
		log_warn("Error reading uploading base64 input");
		free(base64encoded);
		return;
	} 
	if (base64encoded == NULL)
	{
		return;
	}
	if (strlen(base64encoded) > MAX_FILE_UPLOAD_LEN)
	{
		free(base64encoded);
		printf("File is too long!\n");
		return;
	}
	//decode and write to file:
	size_t len = 0; //TODO not completely working!
	char *input = (char *) b64_decode_ex(base64encoded, strlen(base64encoded), &len);
	//int len = strlen(input);
	//len = strlen(base64encoded); //TODO looks better but why?
	log_trace("Inputted length: %i", strlen(base64encoded));
	log_trace("Decoded %i bytes of original base64 file", len);
	free(base64encoded);

	if (len <= 0)
	{
		log_warn("Error parsing the b64: %s", base64encoded);
		printf("Error parsing the b64. Is the uploaded string maximum %i bytes long?\n", MAX_FILE_UPLOAD_LEN);
		free(input);
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
		free(input);
		return;
	}

	size_t written = fwrite(input, 1, len, fp);
	if (written != len)
	{
		printf("Error writing to file\n");
		log_warn("Error writing to file");
		free(input);
		return;
	}
	fclose(fp); //TODO error handling here
	free(input);
	log_trace("Success uploaded file to %s", file_name_complete);

	//TODO error checking
	printf("Success\n");
}

void upload_pcm_file_call()
{
	log_trace("Calling .pcm uploading");
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
		log_warn("File call cancelled: file contains illegal chars '%s'", file_name);
		printf("Error - filename contains illegal character\n");
		return NULL;
	}
	//check if filename ending is correct
	char *dot = strrchr(file_name, '.');
	if (!(dot && !strcmp(dot, file_ending)))
	{
		log_warn("Download pcm aborted, filename does not end with %s: '%s'", file_ending, file_name);
		printf("Error - filename does not end with %s\n", file_ending);
		return NULL;
	}
	//check if filename is not too short or too long
	if (strlen(file_name) < MIN_FILENAME_LEN || strlen(file_name) > MAX_FILENAME_LEN)
	{
		log_warn("File download cancelled, filename has invalid length.");
		printf("File download cancelled, filename has invalid length. Only between %i - %i is allowed.\n", MIN_FILENAME_LEN, MAX_FILENAME_LEN);
		return NULL;
	}
	
	log_trace("Valid filename");
	
	return file_name;
}

void download_file_call(const char* ending)
{
	log_trace("Entering download file call %s", ending);
	char* file_name = ask_correct_filename(ending);
	if (!file_name)
	{
		return;
	}
	log_trace("Download file call for '%s'", file_name);

	//build path with filename
	char* path = build_user_path(file_name);
	printf("read file from path %s\n", path);
	char* path_cpy = strdup(path);
	
	if (!strcmp(file_name, "bach.wav"))
	{
		free(path_cpy);
		path_cpy = (char *) calloc(64, sizeof(char));
		strcpy(path_cpy, "default_data/bach.wav");
		//path_cpy = "default_data/bach.wav";
	}

	//get file content, read_pcm returns the complete binary data
	char *p_buf;
	int file_len = read_pcm(path_cpy, &p_buf);
	if (file_len == -1)
	{
		log_error("Error in read_pcm ocurred");
		printf("Error reading .pcm file\n");
		free(path_cpy);
		return;
	}
	int approx_new_len = base64encode_len(file_len);
	if (approx_new_len >= MAX_FILE_DOWNLOAD_LEN)
	{
		log_warn("Download failed due to too big file size of %i instead of %i\n", approx_new_len, MAX_FILE_DOWNLOAD_LEN);
		printf("Asked file is too big for downloading. Maximum supported size is %i\n", MAX_FILE_DOWNLOAD_LEN);
		free(p_buf);
		free(path_cpy);
		return;
	}
	//b64 encode
	char *encoded = b64_encode((unsigned char*)p_buf, file_len);
	if (encoded)
	{
		printf("File: \n%s\n", encoded);
		log_info("Successfully sent file");
	} else {
		printf("Error\n");
		log_warn("Error encoding b64");
	}
	
	
	free(encoded);
	free(p_buf);
	free(path_cpy);
}

static void download_wav_file_call()
{
	download_file_call(".wav");
}

static void download_pcm_file_call()
{
	download_file_call(".pcm");
}

static void granulize_info_call()
{
	if (last_granular_info)
	{
		print_granular_info(last_granular_info);
	} else {
		printf("No last granular infos to print\n");
	}
}

static void set_option_granular_rate()
{
	const int MIN_OPTION_GRANULAR_RATE = 2;
	const int MAX_OPTION_GRANULAR_RATE = 200;

	log_trace("Set option granular rate");

	char* in = ask("Number of grains per second: (default 10) ");
	int num = atoi(in);
	if (num == 0)
	{
		printf("Error for numerical input\n");
		log_error("Error for input of set option granular rate");
		return;
	}
	
	if (num < MIN_OPTION_GRANULAR_RATE || num > MAX_OPTION_GRANULAR_RATE)
	{
		printf("Error, input has to be between %i and %i\n", 
			MIN_OPTION_GRANULAR_RATE, MAX_OPTION_GRANULAR_RATE);
		log_error("Input out of range");
		return;
	}
	extern int target_grains_per_s;
	target_grains_per_s = num;

	log_info("Successful set option granular rate to %i", num);
	printf("ok\n");
}

static void set_option_grain_timelength()
{
	const int MIN_OPTION_TIMELENGTH = 1;
	const int MAX_OPTION_TIMELENGTH = 10;

	log_trace("Set option grain timelength");

	char* in = ask("New timelength of sample: (default 2) ");
	int num = atoi(in);
	if (num == 0)
	{
		printf("Error for numerical input\n");
		log_error("Error for input of set option granular rate");
		return;
	}
	
	if (num < MIN_OPTION_TIMELENGTH || num > MAX_OPTION_TIMELENGTH)
	{
		printf("Error, input has to be between %i and %i\n", 
			MIN_OPTION_TIMELENGTH, MAX_OPTION_TIMELENGTH);
		log_error("Input out of range");
		return;
	}
	extern int grain_timefactor_scale;
	grain_timefactor_scale = num;

	log_info("Successful set option grain timelength to %i", num);
	printf("ok\n");
}

static void set_option_volume()
{
	const int MIN_OPTION_VOLUME = 1;
	const int MAX_OPTION_VOLUME = 100;

	log_trace("Set option volume");

	char* in = ask("New volume of sample: (default 100) ");
	int num = atoi(in);
	if (num == 0)
	{
		printf("Error for numerical input\n");
		log_error("Error for input of set option volume");
		return;
	}
	
	if (num < MIN_OPTION_VOLUME || num > MAX_OPTION_VOLUME)
	{
		printf("Error, input has to be between %i and %i\n", 
			MIN_OPTION_VOLUME, MAX_OPTION_VOLUME);
		log_error("Input out of range");
		return;
	}
	extern int sample_volume;
	sample_volume = num;

	log_info("Successful set option volume to %i", num);
	printf("ok\n");
}

static void sharing_allow()
{
	log_trace("Sharing allow call");
	if (!sharing_is_allowed(current_user))
	{
		sharing_allow_call(current_user);
	} else {
		printf("Sharing is already allowed\n");
	}
	
}

static void sharing_disallow()
{
	sharing_disallow_call(current_user);
}

static void sharing_use_key()
{
	char *username = ask("Access user: ");
    char *user_dup = strdup(username);
	log_trace("Entered user name: %s", username);

	char *key = ask("Access key: ");
	char *key_dup = strdup(key);
	log_trace("Entered access key: %s\n", key);

	char *file_name = ask("Which file would you like to access: ");
	char *file_name_dup = strdup(file_name);
	log_trace("Entered file name: %s", file_name);
	
	if (path_contains_illegal_chars(user_dup) || path_contains_illegal_chars(file_name_dup))
	{
		printf("Input contains illegal characters\n");
		free(user_dup);
		free(key_dup);
		free(file_name_dup);
		return;
	}

	if (strlen(username) > MAX_USER_NAME_LEN || 
		strlen(key) > 2 * SHA256_SIZE_BYTES || //2 * SHA256 since 1 byte = 2 hex bytes in representation
		strlen(file_name) > MAX_FILENAME_LEN)
	{
		printf("Provided input too long\n");
		return;
	}

	sharing_use_key_call(current_user, user_dup, key_dup, file_name_dup, &last_granular_info);

	free(user_dup);
	free(key_dup);
	free(file_name_dup);
}

static void help_call()
{
	printf("upload wav - uploads a .wav file into own profile, encoded as base64\n");
	printf("upload pcm - uploads a .pcm (pulse-code modulation) file into own profile, encoded as base64\n");
	printf("download wav - downloads a .wav file from own profile, encoded as base64\n");
	printf("download pcm - downloads a .pcm file from own profile, encoded as base64\n");
	printf("granulize - performs granulization algorithm with random parameters on .pcm or .wav file\n");
	printf("granulize info - more details about last granulization process\n");
	printf("set option granular_rate - sets the number of grains per second for a wave file. 10 is the default value\n");
	printf("set option grain timelength - sets the length of the new sample. 2 is the default value.\n");
	printf("set option volume - sets the volume of the output sample. Has to be between 1 - 100. 100 is the default value, which is the highest volume\n");
	printf("sharing allow - allows access for of own granulized files for other users. A key will be generated and prompted which can be used to access the personal account.\n");
	printf("sharing disallow - disallows the sharing (default = disallow).\n");
	printf("sharing use key - uses a key to access other users shared granulized files\n");
	printf("help - this prompt\n");
	printf("quit - quits (surprise)\n\n");
	printf("There is a sample file for granulizing already included! Try out granulizing the file 'bach.wav'. Download the original, then granulize it and download, and compare the results which each other to see how this program works!\n\n");
}

int main()
{	

	alarm(120);
	
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL,  _IONBF, 0);
    
	printf("  _____ _____            _   _ _    _ _      _____ ____________ _____  \n");
	printf(" / ____|  __ \\     /\\   | \\ | | |  | | |    |_   _|___  /  ____|  __ \\ \n");
	printf("| |  __| |__) |   /  \\  |  \\| | |  | | |      | |    / /| |__  | |__) | \n");
	printf("| | |_ |  _  /   / /\\ \\ | . ` | |  | | |      | |   / / |  __| |  _  / \n");
	printf("| |__| | | \\ \\  / ____ \\| |\\  | |__| | |____ _| |_ / /__| |____| | \\ \\ \n");
	printf(" \\_____|_|  \\_\\/_/    \\_\\_| \\_|\\____/|______|_____/_____|______|_|  \\_\\ \n\n");
	
	setup_service();

	while (1)
	{
		char* in = ask("Hello! Do you want to login (l) or register (r)?\n >");
		if (!strcmp(in, "register") || !strcmp(in, "r"))
		{
			register_user();
		} else if (!strcmp(in, "login") || !strcmp(in, "l"))
		{
			bool worked = login_user();
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
		{ "set option granular_rate\n", set_option_granular_rate },
		{ "set option grain timelength\n", set_option_grain_timelength },
		{ "set option volume\n", set_option_volume },
		{ "sharing allow\n", sharing_allow },
		{ "sharing disallow\n", sharing_disallow },
		{ "sharing use key\n", sharing_use_key },
		{ "help\n", help_call },
		{ "quit\n", quit_call }
	};

	char cmd[32];
	while (1)
	{
		printf("What do you want to do?\n > ");
		char *status = fgets(cmd, 32, stdin);
		if (!status)
		{ //In this case an EOF was detected, then exit the program
			quit_call();
		}
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
