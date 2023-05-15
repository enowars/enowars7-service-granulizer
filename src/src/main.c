#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "synth.h"
#include "base64.h"

#define ARRSIZE(a) (sizeof(a)/sizeof(a[0]))

#define WAVE_HEADER_OFFSET 0x44

//Flag for debugging, forces the creation of a new clean setup when service is started
#define FORCE_NEW_SETUP false

#define MAX_LEN_USER_FILE 65536
char user_file_content[MAX_LEN_USER_FILE];

typedef struct granular_info {
	int num_samples;
	int* order_samples;
	int* order_timelens;
} granular_info;

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

/*
 *
 * Read users-info.txt into user_file_content array
 *
 * @return 	0 is success
			1 if file couldn't be opened
			2 error reading file
 */
int load_user_file()
{
	FILE* fp = fopen("users-info.txt", "r");
	if (!fp)
	{
		printf("couldnt open file\n");
		return 1; //error
	}
	int len = fread(user_file_content, MAX_LEN_USER_FILE, 1, fp);
	if (len == 0 && ferror(fp))
	{
		printf("error reading file\n");
		return 2;
	}

	fclose(fp);
	
	printf("Info, loaded file: %s\n", user_file_content);

	return 0;
}

bool exist_username_with_password(char* username_in, char* password_in)
{
	char delimiter[] = ";";
	char delimiter_details[] = ":";
	char *save_ptr_1, *save_ptr_2;

	//USER:PASSWORD:PERSONAL_INFO
	if (!user_file_content) return false; //if no users exist return false


	char* split = strdup(user_file_content);
	
	//split
	char* data_row = strtok_r(split, delimiter, &save_ptr_1);

	while (data_row)
	{
		
		if (data_row[0] == '\n') return false;

		printf("splitted %s\n", split);
		
		
		//split content
		char* username 	= strtok_r(data_row, delimiter_details, &save_ptr_2);
		assert(username);
		char* pwd 		= strtok_r(NULL, delimiter_details, &save_ptr_2);
		assert(pwd);
		char* details 	= strtok_r(NULL, delimiter_details, &save_ptr_2);
		assert(details);
		printf("user 	%s\n", username);
		printf("pwd 	%s\n", pwd);
		printf("details %s\n", details);

		if (!strcmp(username, username_in))
		{
			if (password_in)
			{ //only check password if one was specified
				return (!strcmp(pwd, password_in));
			} else {
				return true;
			}
		}

		//get next data_row
		printf("split %s\n", split);
		data_row = strtok_r(NULL, delimiter, &save_ptr_1);
		printf("row, %s\n", data_row);
	}
	return false;
/*
	user_file_content_cpy = strtok(user_file_content_cpy, delimiter);
	ptr_cpy = strdup(user_file_content_cpy);

	do { //parse each line
		printf("1\n");

		
		char* username 	= strtok(ptr_cpy, delimiter_details);
		assert(username);
		char* pwd 		= strtok(ptr_cpy, delimiter_details);
		assert(pwd);
		char* details 	= strtok(ptr_cpy, delimiter_details);
		assert(details);

		printf("parsed username %s\n", username);
		printf("in comparison with %s\n", username_in);

		if (!strcmp(username, username_in))
		{
			if (password_in)
			{ //only check password if one was specified
				return (!strcmp(pwd, password_in));
			} else {
				return true;
			}
		}



	user_file_content_cpy = strtok(user_file_content_cpy, delimiter);
	ptr_cpy = strdup(user_file_content_cpy);

	} while (ptr_cpy);

	return false;
	*/
}

bool exist_username(char* username_in)
{
	return exist_username_with_password(username_in, NULL);
}

int add_user(char* username, char* pwd, char* details)
{	
	//open file and append user infos
	FILE* fp = fopen("users-info.txt", "a");
	if (!fp)
	{
		printf("couldnt open file\n");
		return 1; //error
	}
	int len = fwrite(username, strlen(username), 1, fp);
	if (len < 1)
	{
		printf("error writing file: %i\n", len);
		return 2;
	}
	len = fwrite(":", 1, 1, fp);
	if (len < 1)
	{
		printf("error writing file: %i\n", len);
		return 2;
	}
	len = fwrite(pwd, strlen(pwd), 1, fp);
	if (len < 1)
	{
		printf("error writing file: %i\n", len);
		return 2;
	}
	len = fwrite(":", 1, 1, fp);
	if (len < 1)
	{
		printf("error writing file: %i\n", len);
		return 2;
	}
	len = fwrite(details, strlen(details), 1, fp);
	if (len < 1)
	{
		printf("error writing file: %i\n", len);
		return 2;
	}
	len = fwrite(";", 1, 1, fp);
	if (len < 1)
	{
		printf("error writing file: %i\n", len);
		return 2;
	}


	fclose(fp);
	return 0;
}

void login()
{
	char* username  = ask("Username: ");
	char* password	= ask("Password: ");
	printf("Checking %s with %s\n", username, password);
	
	load_user_file(); //load current user file
	if (exist_username_with_password(username, password))
	{
		printf("Welcome %s!\n", username);
	} else {
		printf("Wrong password\n");
		exit(0);
	}
	return;
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

	return;
}



void print_granular_info(granular_info* info)
{
	printf("Granular_number_samples = %i;\n", info->num_samples);
	printf("Granular_order_samples = {");
	for (int i=0; i < info->num_samples - 1; i++)
	{
		printf("%i,", info->order_samples[i]);
	}
	printf("%i}\n", info->order_samples[info->num_samples - 1]);

	printf("Granular_order_timelens = {");
	for (int i=0; i < info->num_samples - 1; i++)
	{
		printf("%i,", info->order_timelens[i]);
	}
	printf("%i}\n", info->order_samples[info->num_samples - 1]);
}

granular_info* synth(char* buf, int buf_len)
{
	const int num_grains = 100;
	
	granular_info* info = malloc(sizeof(granular_info));
	info->num_samples = num_grains;
	info->order_samples 	= calloc(num_grains, sizeof(char));
	info->order_timelens 	= calloc(num_grains, sizeof(char));
	
	int grains_len = buf_len / num_grains;

	int left_grains[100];
	for (int i=0; i < num_grains; i++)
	{ //create array for choosing random index from it
		left_grains[i] = i;
	}

	unsigned char is_used[100] = { 0 }; /* flags */
	int i = 0;
	for (i = 0; i < num_grains; i++) {
		int r = rand() % (i + 1); /* generate a random number 'r' */

		if (is_used[r])
			/* we already have 'r' */
			r = i; /* use 'in' instead of the generated number */

		assert(!is_used[r]);
		info->order_samples[i++] = r;
		is_used[r] = 1; //flag as already used
	}
	
	assert(i == num_grains);

	/*
	for (int i = num_grains; i >= 0; i--)
	{ //pick random grain from left_grains
		int index = i > 0 ? rand() % (i+1) : 0;

		printf("%i, ", index);
		//left_grains[index];
		info->order_samples[100 - i] = index;
	}*/

	return info;
}

void synth_file_call()
{
	printf("Enter a file name: ");
	char file_name[1024];
	fgets(file_name, 1024, stdin);
	char* tok = strchr(file_name, '\n');
	if (tok) *tok = '\0';

	

	//check that file name is valid
	char *dot = strrchr(file_name, '.');
	if ( !dot || strcmp(dot, ".wav"))
	{
		printf("file name has to end with .wav\n");
		return;
	}
	//and "file is in correct folder" (fake check)

	//open file
	char* fn = strdup(file_name);
	if (!fn)
	{
		return;
	}

	#define MAXBUFLEN 100000

	char file_content[MAXBUFLEN + 1];
	FILE* fp = fopen(fn, "rb");
	if (!fp)
	{
		printf("error opening file\n");
		return;
	}
	int len = fread(file_content, sizeof(char), MAXBUFLEN, fp);
	if (len <= 0)
	{
		printf("error reading file\n");
		return;
	}
	printf("%s", file_content);
	fclose(fp);

	granular_info* info = synth(file_content, len);
	print_granular_info(info);
}

void upload_wav_file_call()
{
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

	FILE* fp = fopen("output.wav", "wb");
	if (!fp)
	{
		perror("fopen");
		return 1;
	}

	fwrite(input, 1, len, fp);
	fclose(fp);

}


int main()
{
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
	}
	exit(0);

	init();

	struct {
		const char *name;
		void (*func)();
	} cmds[] = {
		{ "login\n", login },
		{ "register\n", reg },
		{ "upload wav\n", upload_wav_file_call },
		{ "synth\n", synth_file_call }
		/*{ "users", api_list_users },
		{ "info", api_user_info },
		{ "login", api_login },
		{ "post", api_create_post },
		{ "posts", api_list_posts },
		{ "help", api_help },
		*/
	};

	printf("Hello synthy!\n");
	

	//printf("Thanks for logging in as '%s'\n", user);
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
