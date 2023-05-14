#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "synth.h"
#include "base64.h"

#define ARRSIZE(a) (sizeof(a)/sizeof(a[0]))

#define WAVE_HEADER_OFFSET 0x44

typedef struct granular_info {
	int num_samples;
	int* order_samples;
	int* order_timelens;
} granular_info;

void init()
{
	srand(time(NULL)); //create random seed
}

char* ask(const char* prompt)
{
	printf("%s\n", prompt);

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
	init();

	struct {
		const char *name;
		void (*func)();
	} cmds[] = {
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
	
	//testcase, suppose the user already logged in
	char* user = "admin\0";

	printf("Thanks for logging in as '%s'\n", user);
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
