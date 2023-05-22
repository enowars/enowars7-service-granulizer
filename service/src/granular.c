#include "granular.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "log.c/log.h"

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

int scale_array_custom_sample_length(char* buf_in, char** buf_out, int buf_in_len, int factor, int bytes_per_sample)
{
    //printf("buf_in_len: %i, factor: %i\n", buf_in_len, factor);
    if (factor < 1) {
        return -1;
    }

    int num_samples = buf_in_len / bytes_per_sample;
    int offset = 0;
    //create new array which is "factor" bigger
    char *buf = malloc(buf_in_len * sizeof(char) * factor);

    for (int i=0; i < num_samples; i++)
    {
        for (int j=0; j < factor; j++) {
            memcpy(buf + offset, buf_in + i * bytes_per_sample, bytes_per_sample);
            offset += bytes_per_sample;
        }
    }

    *buf_out = buf;
    return factor * buf_in_len;
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

void print_granular_info(const granular_info* info)
{
	printf("granular_number_samples = %i\n", info->num_samples);
    printf("grain_len = %i\n", info->grain_len);
	printf("granular_order_samples = [");
	for (int i=0; i < info->num_samples - 1; i++)
	{
		printf("%i,", info->order_samples[i]);
	}
	printf("%i]\n", info->order_samples[info->num_samples - 1]);

	printf("granular_order_timelens = [");
	for (int i=0; i < info->num_samples - 1; i++)
	{
		printf("%i,", info->order_timelens[i]);
	}
	printf("%i]\n", info->order_timelens[info->num_samples - 1]);

	printf("granular_order_buffer_lens = [");
	for (int i=0; i < info->num_samples - 1; i++)
	{
		printf("%i,", info->order_buffer_lens[i]);
	}
	printf("%i]\n", info->order_buffer_lens[info->num_samples - 1]);
    
}



granular_info* granulize(const char* buf, const int buf_len, char** buf_out, int* len_out, const unsigned int bytes_per_sample)
{
    log_trace("Starting granulize algorithm");

	int num_grains = 5;
	if (buf_len < num_grains)
    {
        num_grains = buf_len;
    }

    int grains_len = buf_len / num_grains;

	granular_info* info = malloc(sizeof(granular_info));
	info->num_samples = num_grains;
	info->order_samples 	= calloc(num_grains, sizeof(int));
	info->order_timelens 	= calloc(num_grains, sizeof(int));
    info->order_buffer_lens = calloc(num_grains, sizeof(int));
	info->grain_len = grains_len;

	
    //random order of samples
	for (int i=0; i < num_grains; i++)
	{
		info->order_samples[i] = i;
	}
    shuffle(info->order_samples, num_grains);

    //random length of each sample
    #define num_possible_sample_lengths 1
    int possible_sample_lengths[num_possible_sample_lengths] = {2};
    
    int sample_position = 0;
    int new_sample_len = 0; //length of new sample

    for (int i=0; i < num_grains; i++)
    {
        info->order_timelens[i] = possible_sample_lengths[rand() % num_possible_sample_lengths];
        info->order_buffer_lens[i] = info->grain_len; //write correct buffer len for this grain
        int grains_len_here = grains_len;
        //change grains len if it is the last element
        if (i == (num_grains - 1))
        {
            //info->order_timelens = new_sample_
            info->order_buffer_lens[i] = (buf_len - sample_position);
            printf("Timelen is here %i\n", info->order_timelens[i]);
            printf("Buf Len: %i\n", buf_len);
            printf("Sample len: %i\n", sample_position);
            printf("Buffer length of last (weird) element: %i\n", info->order_buffer_lens[i]);
            printf("Time factor of last (weird) element: %i\n", info->order_timelens[i]);
            assert(info->order_buffer_lens[i] + (num_grains - 1) * grains_len == buf_len);
        }
        //printf("Grains len here %i for %i\n", grains_len_here, i);
        new_sample_len += info->order_buffer_lens[i] * info->order_timelens[i]; //calculate new sample size
        sample_position += grains_len_here;
    }
    
    //granular info is created, now granulize
    
    char* new_sample = calloc(new_sample_len, sizeof(char));

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
        //int res = scale_array_custom_sample_length(buf + index, &buf_new, grains_len_here, info->order_timelens[i], bytes_per_sample);
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
