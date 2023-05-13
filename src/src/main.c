#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "synth.h"
#include "base64.h"

#define ARRSIZE(a) (sizeof(a)/sizeof(a[0]))

void synth_file_call()
{
	printf("Enter a file name: ");
	char file_name[1024];
	fgets(file_name, 1024, stdin);

	//check that file name is valid
	char *dot = strrchr(file_name, '.');
	if ( !(dot && !strcmp(dot, ".wav\n")))
	{
		printf("file name has to end with .wav\n");
		return;
	}
	//and "file is in correct folder" (fake check)



}

void upload_file_call()
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

	struct {
		const char *name;
		void (*func)();
	} cmds[] = {
		{ "upload\n", upload_file_call },
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
