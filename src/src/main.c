#include <stdio.h>
#include <stdbool.h>
#include "music_builder.h"

int main()
{
	printf("Hello synthy!\n");
	
	//testcase, suppose the user already logged in
	char* user = "admin\0";

	printf("Thanks for logging in as '%s'\n", user);
	printf("What do you wanna do?\n > ");
	
	char input[32];
	fgets(input, 32, stdin);

	if (strcmp(input, "SET MELODY") == 0)
	{
		printf("Set melody!\n");
	} else {
		printf("Cant find that:(\n");
	}



	//basic testing
	track_info* t = track_info_create();
	if (!t)
	{
		printf("Error initializing track\n");
		return -1;
	}




	track_info_destroy(t);

}
