#include "granular.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void shuffle(int *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

int scale_array(char* buf_in, char** buf_out, int buf_in_len, int factor)
{
    //printf("buf_in_len: %i, factor: %i\n", buf_in_len, factor);
    if (factor < 1) {
        return -1;
    }

    //create new array which is "factor" bigger
    char *buf = malloc(buf_in_len * sizeof(char) * factor);
    for (int i=0; i < buf_in_len; i++)
    {
        memset(buf + i * factor, buf_in[i], factor);
        //printf("Set char %c (%i) number of times: %i\n", buf_in[i], buf_in[i], buf_in_len);
    }

    *buf_out = buf;
    return factor * buf_in_len;
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

granular_info* granulize(char* buf, int buf_len, char** buf_out, int* len_out)
{
    //printf("Input buf len: %i\n", buf_len);
    
	int num_grains = 100;
	if (buf_len < num_grains)
    {
        num_grains = buf_len;
    }

	granular_info* info = malloc(sizeof(granular_info));
	info->num_samples = num_grains;
	info->order_samples 	= calloc(num_grains, sizeof(int));
	info->order_timelens 	= calloc(num_grains, sizeof(int));
	
	int grains_len = buf_len / num_grains;
    int new_sample_len = 0;

    //random order of samples
	for (int i=0; i < num_grains; i++)
	{
		info->order_samples[i] = i;
	}
    shuffle(info->order_samples, num_grains);

    //random length of each sample
    #define num_possible_sample_lengths 5
    int possible_sample_lengths[num_possible_sample_lengths] = {1, 2, 4, 8, 16};
    for (int i=0; i < num_grains; i++)
    {
        info->order_timelens[i] = possible_sample_lengths[rand() % num_possible_sample_lengths];
        int grains_len_here = grains_len;
        //change grains len if it is the last element
        if (i == (num_grains - 1))
        {
            grains_len_here = buf_len - (grains_len * (num_grains - 1));
        }
        //printf("Grains len here %i for %i\n", grains_len_here, i);
        new_sample_len += grains_len_here * info->order_timelens[i]; //calculate new sample size
    }
    
    //granular info is created, now granulize
    
    char* new_sample = calloc(new_sample_len, sizeof(char));
    //printf("new sample len %i\n", new_sample_len);

    int next_index_for_writing = 0;
    for (int i=0; i < num_grains; i++)
    {
        //add grain

        //create new longer sample, scaled as wished by new timelength
        char* buf_new;
        int index = info->order_samples[i] * grains_len;
        int grains_len_here = grains_len;
        if (info->order_samples[i] == (num_grains - 1))
        { //special case for last grain, this has a different length
            grains_len_here = buf_len - (grains_len * (num_grains -1));
            //printf("special grain len: %i\n", grains_len_here);
        }
        int res = scale_array(buf + index, &buf_new, grains_len_here, info->order_timelens[i]);
        //TODO proper error handling
        if (res)
        {

        }
        memcpy(new_sample + next_index_for_writing, buf_new, grains_len_here * info->order_timelens[i]);
        next_index_for_writing += grains_len_here * info->order_timelens[i];
        
    }

    *buf_out = new_sample;
    *len_out = new_sample_len;

	return info;
}