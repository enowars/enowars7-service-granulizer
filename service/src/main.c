#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "users.h"
#include "base64.h"
#include "granular.h"
#include "file_handler.h"
#include "log.c/log.h"



#define ARRSIZE(a) (sizeof(a)/sizeof(a[0]))

//With .pcm / .wav ending, therefore a filename needs to have a minimum length of 5
#define MIN_FILENAME_LEN ((int) 5)
#define MAX_FILENAME_LEN ((int) 64 + 5)

#define MAX_FILE_UPLOAD_LEN 500000
//1Mb of maximum file size for downloading
#define MAX_FILE_DOWNLOAD_LEN 1048576

//Flag for debugging, forces the creation of a new clean setup when service is started
#define FORCE_NEW_SETUP false

char* current_user = NULL;
granular_info* last_granular_info = NULL;

static void quit_call()
{
	//frees all used memory
	if (current_user)
	{
		free(current_user);
		current_user = NULL;
	}
	destroy_granular_info(last_granular_info);
	
	log_info("Freed all memory");
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
		//quit_call();
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
	//srand(time(NULL)); //create random seed

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
	if (!fp)
	{
		return;
	}
	fclose(fp);

	

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
		log_trace("Couldnt create user '%s' - already exists", username_cpy);
	} else {
		add_user(username_cpy, password_cpy);
		printf("ok\n");
		log_trace("User '%s' created successfully", username_cpy);
	}
	free(username_cpy);
	free(password_cpy);

}

void granulize_call()
{
	enum FILE_MODE {
		PCM,
		WAV
	};
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

	//check that file name is valid
	char *dot = strrchr(file_name, '.');
	if (!dot)
	{
		printf("File ending is missing\n");
		return;
	}
	if (strcmp(dot, ".wav") && strcmp(dot, ".pcm"))
	{
		printf("file has to end with .wav or .pcm\n");
		return;
	}
	log_debug("Correct file ending: %s", dot);

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

	int len;
	char* p_data;
	WavHeader* w_header = NULL;
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
			log_error("No data from read_pcm, abort granulization");
			printf("Error reading file\n");
			return;
		}
	} else {
		log_error("no pcm or wav file for input, instead: %s", dot);
		printf("Error, wrong file format");
		return;
	}
	log_info("Successfully read data");

	//handle data:
	char *new_sample;
	int new_sample_len;
	int bytes_per_sample;
	int samplerate;
	if (w_header)
	{
		log_trace("Wheader available");
		samplerate = w_header->SampleRate;
		bytes_per_sample = w_header->BitsPerSample / 8;
		log_info("Bytes per sample read: %i", bytes_per_sample);
	} else {
		log_trace("Wheader not available");
		samplerate = 10;
		bytes_per_sample = 1;
	}

	granular_info* info = granulize(p_data, len, &new_sample, &new_sample_len, bytes_per_sample, samplerate);
	if (!info)
	{
		printf("Error granulizing\n");
		if (w_header)
		{
			free(w_header);
			w_header = NULL;
		}
		if (p_data)
		{
			free(p_data);
			p_data = NULL;
		}
		return;
	}
	if (p_data)
	{
		free(p_data);
		p_data = NULL;
	}

	if (last_granular_info)
	{
		destroy_granular_info(last_granular_info);
		last_granular_info = NULL;
	}
	last_granular_info = info;

	//write data to users folder
	memset(file_name_complete, 0, 128);
	strcpy(file_name_complete, "users/");
	strcat(file_name_complete, current_user);
	strcat(file_name_complete, "/");
	
	
	//write file
	if (file_mode == WAV)
	{
		strcat(file_name_complete, "granulized.wav");
		write_wav(file_name_complete, new_sample, w_header, new_sample_len);
	} else if (file_mode == PCM)
	{
		strcat(file_name_complete, "granulized.pcm");
		write_pcm(file_name_complete, new_sample, new_sample_len);
	} else {
		printf("Error, wrong file format");
		log_error("no pcm or wav file for input, instead: %s", dot);
		if (new_sample)
		{
			free(new_sample);
		}
		return;
	}

	if (new_sample)
	{
		free(new_sample);
	}
	if (w_header)
	{
		free(w_header);
		w_header = NULL;
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

void upload_file(const char* ending)
{
	//log_trace("Upload file (with ending) call");

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
	char *base64encoded = readline(NULL); //readline is important to use due to the big buffer size
	
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

	char input[MAX_FILE_UPLOAD_LEN];
	//decode and write to file:
	int len = Base64decode(input, base64encoded);
	log_trace("Inputted length: %i", strlen(base64encoded));
	log_trace("Decoded %i bytes of original base64 file", len);
	free(base64encoded);
	if (len <= 0)
	{
		log_warn("Error parsing the b64: %s", base64encoded);
		printf("Error parsing the b64. Is the uploaded string maximum %i bytes long?\n", MAX_FILE_UPLOAD_LEN);
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
	int approx_new_len = Base64encode_len(file_len);
	if (approx_new_len >= MAX_FILE_DOWNLOAD_LEN)
	{
		log_warn("Download failed due to too big file size of %i instead of %i\n", approx_new_len, MAX_FILE_DOWNLOAD_LEN);
		printf("Asked file is too big for downloading. Maximum supported size is %i\n", MAX_FILE_DOWNLOAD_LEN);
		free(p_buf);
		free(path_cpy);
		return;
	}
	//b64 encode
	char encoded[MAX_FILE_DOWNLOAD_LEN]; //currently support 1Mb for debugging
	file_len = Base64encode(encoded, p_buf, file_len);
	printf("File: \n%s\n", encoded);
	log_info("Successfully sent file");
	
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
