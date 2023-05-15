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

int scale_array(char* buf_in, char* buf_out, int buf_in_len, int factor)
{
    if (factor <= 1) {
        return -1;
    }

    //create new array which is "factor" bigger
    *buf_out = malloc(buf_in_len * sizeof(char) * factor);
    for (int i=0; i < buf_in_len; i++)
    {
        memset(buf_out + i * factor, buf_in[i], factor);
    }

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
    printf("Input buf len: %i\n", buf_len);
    
	const int num_grains = 100;
	
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
        printf("Grains len here %i for %i\n", grains_len_here, i);
        new_sample_len += grains_len_here * info->order_timelens[i]; //calculate new sample size
    }
    
    //granular info is created, now granulize
    
    char* new_sample = calloc(new_sample_len, sizeof(char));
    printf("new sample len %i\n", new_sample_len);
    char* ptr = new_sample;
    int index = 0;
    int next_index_for_writing = 0;
    for (int i=0; i < num_grains; i++)
    {
        //add grain

        //create new longer sample, scaled as wished by new timelength
        char buf_new;
        int index = info->order_samples[i] * grains_len;
        int grains_len_here = grains_len;
        if (info->order_samples[i] == (num_grains - 1))
        { //special case for last grain, this has a different length
            grains_len_here = buf_len - (grains_len * (num_grains -1));
            printf("special grain len: %i\n", grains_len_here);
        }
        int res = scale_array(buf + index, &buf_new, grains_len_here, info->order_timelens[i]);
        
        memcpy(new_sample + next_index_for_writing, &buf_new, grains_len_here * info->order_timelens[i]);
        next_index_for_writing += grains_len_here * info->order_timelens[i];
        
    }

    *buf_out = new_sample;
    *len_out = new_sample_len;

	return info;
}