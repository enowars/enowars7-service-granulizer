#include "granular.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

granular_info* granulize(char* buf, int buf_len)
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