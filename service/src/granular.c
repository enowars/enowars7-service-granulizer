#include "granular.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "log.c/log.h"


void grain_print(grain *g)
{
    if (g)
    {
        printf("Buffer pointer: %p\n", g->buf);
        printf("Grain size: %i\n", g->buf_len);
    }
}

void grain_print_complete(grain *g)
{
    if (g)
    {
        printf("Buffer pointer: %p\n", g->buf);
        printf("Grain buf_len: %i\n", g->buf_len);
        
        printf("Grains original position: %i\n", g->orig_pos);
        printf("Grains applied timefactor: %i\n", g->used_time_factor);
        if (g->buf_len >= 1)
        {
            printf("Grain buffer: [");
            for (int i=0; i < g->buf_len - 1; i++)
            {
                printf("%i, ", g->buf[i]);
            }
            printf("%i]\n", g->buf[g->buf_len - 1]);
        }
        printf("\n");
    }
}

static grain* grain_create()
{
    grain *back = calloc(1, sizeof(grain));
    if (!back)
    {
        return NULL;
    }
    return back;
}

static void grain_destroy(grain *g)
{
    if (!g)
    {
        return;
    }
    if (g->buf)
    {
        free(g->buf);
        g->buf = NULL;
    }
    free(g);
}

static void shuffle_pointer(void** array, size_t n) {
    srand(time(NULL)); // seed the random number generator

    for (void** i = array + n - 1; i > array; i--) {
        void** j = array + rand() % (i - array + 1);
        void* tmp = *j;
        *j = *i;
        *i = tmp;
    }
}

static int scale_array_custom_sample_length(const char* buf_in, char** buf_out, int buf_in_len, int factor, int bytes_per_sample)
{
    if (factor < 1) {
        return -1;
    }

    int num_samples = buf_in_len / bytes_per_sample;
    int offset = 0;
    //create new array which is "factor" bigger
    char *buf = calloc(buf_in_len * sizeof(char) * factor, 1);

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

void destroy_granular_info(granular_info *g)
{
    if (g)
    {
        if (g->order_samples)
        {
            free(g->order_samples);
            g->order_samples = NULL;
        }
        if (g->order_timelens) 
        {
            free(g->order_timelens);
            g->order_timelens = NULL;
        }
        if (g->order_buffer_lens) 
        {
            free(g->order_buffer_lens);
            g->order_buffer_lens = NULL;
        }
        free(g);
    }
}

void print_granular_info(const granular_info* info)
{
	printf("granular_number_samples = %i\n", info->num_samples);
    //info_trace("granular_number_samples = %i\n", info->num_samples);
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

granular_info* granulize(char* buf, const int buf_len, char** buf_out, int* len_out, 
    const unsigned int bytes_per_sample, const int samplerate)
{
    log_trace("Starting granulize algorithm with params: buf_len %i, bytes_per_sample %i, samplerate: %i", 
        buf_len, bytes_per_sample, samplerate);
    
    //grains_len has to be minimum bytes_per_sample, choose length so it is
    const int target_grains_per_s = 10;
    int minimum_grain_number = (int) (ceil((double) buf_len / (double) bytes_per_sample)); //minimum possible number of grains when bytes_per_sample are still regarded
    //int num_grains = (int)((double)minimum_grain_number / (double)(samplerate * target_grains_per_s));
    int num_grains = (int) ((double)buf_len / (double) samplerate * (double)target_grains_per_s);
    if (num_grains > minimum_grain_number)
    {
        log_trace("Changing num grains due to set limit");
        num_grains = minimum_grain_number;
    }
    log_trace("Minimum grain number: %i, num_grains: %i", minimum_grain_number, num_grains);
    int grain_len = buf_len / num_grains; //length of a normal grain, TODO ändern
    log_trace("grain_len: %i", num_grains, grain_len, minimum_grain_number);
    if ((grain_len % bytes_per_sample) != 0)
    {
        //log_warn("Grain_len (%i) is not a multiple of bytes_per_sample (%i)! This could lead to weird errors", grain_len, bytes_per_sample);
    }
    if (num_grains <= 1)
    {
        return NULL;
    }
    

    //create granular info for returning:
    granular_info* info = malloc(sizeof(granular_info));
	if (!info)
    {
        return NULL;
    }
    info->num_samples = num_grains;
	info->order_samples 	= calloc(num_grains, sizeof(int));
	if (!info->order_samples)
    {
        destroy_granular_info(info);
        return NULL;
    }
    info->order_timelens 	= calloc(num_grains, sizeof(int));
    if (!info->order_timelens)
    {
        destroy_granular_info(info);
        return NULL;
    }
    info->order_buffer_lens = calloc(num_grains, sizeof(int));
    if (!info->order_buffer_lens)
    {
        destroy_granular_info(info);
        return NULL;
    }

    //create grains:
    grain *grains[num_grains];
    for (int i=0; i < num_grains; i++)
    {
        grains[i] = NULL;
    }
    
    for (int i=0; i < num_grains -1; i++)
    { //even sized grains
        grain *g = grain_create();
        if (!g)
        {
            for (int i=0; i < num_grains; i++) //cleanup
            {
                if (grains[i] != NULL)
                {
                    grain_destroy(grains[i]);
                    grains[i] = NULL;
                }
            }
            destroy_granular_info(info);
            log_error("Error creating grain");
            return NULL;
        }
        g->orig_pos = i;

        //create actual grain
        g->buf_len = grain_len;
        g->buf = malloc(g->buf_len * sizeof(char));
        if (!g->buf)
        { //abort
            for (int i=0; i < num_grains; i++) 
            { //cleanup
                if (grains[i] != NULL)
                {
                    grain_destroy(grains[i]);
                    grains[i] = NULL;
                }
            }
            destroy_granular_info(info);
            log_error("Error mallocing buffer");
            return NULL;
        }
        int offset_bytes = i * grain_len * sizeof(char);
        void* p_to_cpy_from = buf + offset_bytes;
        memcpy(g->buf, p_to_cpy_from, g->buf_len * sizeof(char));

        log_trace("New grain created for index %i: %p with buf_len %i", i, g, g->buf_len);
        grains[i] = g;
    }
    //last special shorter grain
    grain *g = grain_create();
    if (!g)
    {
        for (int i=0; i < num_grains; i++) //cleanup
        {
            if (grains[i] != NULL)
            {
                grain_destroy(grains[i]);
                grains[i] = NULL;
            }
        }
        destroy_granular_info(info);
        log_error("Error creating grain");
        return NULL;
    }
    int offset_bytes = (num_grains -1) * grain_len * sizeof(char);
    g->buf_len = buf_len - offset_bytes;
    g->buf = malloc(g->buf_len * sizeof(char));
    if (!g->buf)
    {
        for (int i=0; i < num_grains; i++) 
        { //cleanup
            if (grains[i] != NULL)
            {
                grain_destroy(grains[i]);
                grains[i] = NULL;
            }
        }
        destroy_granular_info(info);
        log_error("Error mallocing buffer");
        return NULL;
    }
    g->orig_pos = num_grains-1;
    log_trace("Create shorter special grain, buf_len %i (offset_bytes %i), orig_pos %i", g->buf_len, offset_bytes, g->orig_pos);
    void* p_to_cpy_from = buf + (num_grains -1) * grain_len;
    memcpy(g->buf, p_to_cpy_from, g->buf_len);
    grains[num_grains - 1] = g;
    //done creating all grains

    //debug grains array:
    /*
    for (int i = 0; i < num_grains; i++)
    {
        grain_print_complete(grains[i]);
    }
    */

    //all original grains are now created
    shuffle_pointer((void*) grains, num_grains);
    //log_trace("Grains shuffled");

    //apply random timefactor for each grain. Timefactor is maximum MAX_TIMEFACTOR, and could be negative
    for (int i = 0; i < num_grains; i++)
    {
        int timefactor = 2; //TODO change timefactor and randomize
        grains[i]->used_time_factor = timefactor;
    }

    int new_buf_len = 0;
    //create new grains with new timefactor
    for (int i = 0; i < num_grains; i++)
    {
        grain *g = grains[i];
        //time factor adjusting
        int abs_time_factor = abs(g->used_time_factor);
        char *buf_new;
        int res_time_factor = scale_array_custom_sample_length(g->buf, &buf_new, 
            g->buf_len, abs_time_factor, bytes_per_sample);
        if (res_time_factor < 0)
        {
            //TODO return with error
        }
        if (abs_time_factor * g->buf_len != res_time_factor)
        {
            log_trace("Abs time factor: %i, real_time_factor: %i", abs_time_factor, res_time_factor);
            log_warn("Scale array did not work due unaligned data. Reset this grains timefactor and length");
            g->used_time_factor = res_time_factor;
            abs_time_factor = res_time_factor;
        }
        //write in new buffer
        free(g->buf);
        g->buf = buf_new;
        log_trace("Old buf len %i", g->buf_len);
        log_trace("Abs time factor: %i", abs_time_factor);
        g->buf_len = g->buf_len * abs_time_factor;
        log_trace("Changed new buf len to %i", g->buf_len);
        new_buf_len += g->buf_len;
    }
    log_trace("New buffer len: %i", new_buf_len);

    int offset = 0;
    char *new_buf = malloc(new_buf_len);
    if (!new_buf)
    {
        for (int i = 0; i < num_grains; i++)
        {
            grain_destroy(grains[i]);
        }
        destroy_granular_info(info);
        log_error("Erro allocating memory for new buffer");
        return NULL;
    }
    //build new grains. Contains original grains and overlapping between
    for (int i = 0; i < num_grains; i++)
    {
        //log_trace("Next grain with index %i", i);
        grain *g_current = grains[i];
        //copy current grain
        memcpy(new_buf + offset, g_current->buf, g_current->buf_len);
        
        offset += g_current->buf_len;
    }

    /*
    for (int i = 0; i < num_grains; i++) //debug
    {
        grain_print_complete(grains[i]);
    }*/

    //write granular_info for returning filter coefficients
    info->num_samples = num_grains;
    for (int i = 0; i < num_grains; i++)
    {
        info->order_samples[i] = grains[i]->orig_pos;
        info->order_timelens[i] = grains[i]->used_time_factor;
        info->order_buffer_lens[i] = grains[i]->buf_len;
    }
    //cleanup
    for (int i = 0; i < num_grains; i++)
    {
        grain_destroy(grains[i]);
    }
    //return
    *len_out = new_buf_len;
    *buf_out = new_buf;
    return info;
}

/**
 * @brief 
 * 
 * @param buf 
 * @param buf_len 
 * @param buf_out 
 * @param len_out 
 * @param bytes_per_sample 
 * @param samplerate Samplerate for .wav file, or -1 if .pcm file. Then the num_grains will be constant
 * @return granular_info* 
 */
/*
granular_info* granulize(const char* buf, const int buf_len, char** buf_out, int* len_out, 
    const unsigned int bytes_per_sample, const int samplerate)
{
    log_trace("Starting granulize algorithm with params: bytes_per_sample %i, samplerate: %i", bytes_per_sample, samplerate);
    
    //grains_len has to be minimum bytes_per_sample, choose length so it is
    const int target_grains_per_s = 10;
    int num_grains;
    if (samplerate > 0)
    { //if valid samplerate is given
        num_grains = (buf_len / samplerate) * target_grains_per_s;
    } else {
        num_grains = 10;
    }

    int grains_len = buf_len / num_grains;
    if (grains_len % bytes_per_sample != 0)
    {
        log_trace("Grains_len before %i", grains_len);
        grains_len -= (grains_len % bytes_per_sample);
        log_info("Adjusting grains len to %i", grains_len);
    }
    log_trace("Input granular bytes length %i", buf_len);
    log_trace("Grains_len now %i", grains_len);
    log_trace("Num grains now %i", num_grains);
    
	granular_info* info = malloc(sizeof(granular_info));
	info->num_samples = num_grains;
	info->order_samples 	= calloc(num_grains, sizeof(int));
	info->order_timelens 	= calloc(num_grains, sizeof(int));
    info->order_buffer_lens = calloc(num_grains, sizeof(int));

	
    //random order of samples
	for (int i=0; i < num_grains; i++)
	{
		info->order_samples[i] = i;
	}
    shuffle(info->order_samples, num_grains);

    //random length of each sample
    #define num_possible_sample_lengths 2
    int possible_sample_lengths[num_possible_sample_lengths] = {2, 4};
    
    int sample_position = 0;
    int new_sample_len = 0; //length of new sample

    for (int i=0; i < num_grains; i++)
    {
        info->order_timelens[i] = possible_sample_lengths[rand() % num_possible_sample_lengths];
        info->order_buffer_lens[i] = grains_len; //write correct buffer len for this grain
        int grains_len_here = grains_len;
        //change grains len if it is the last element
        if (i == (num_grains - 1))
        {
            info->order_buffer_lens[i] = (buf_len - sample_position);
            assert(info->order_buffer_lens[i] + (num_grains - 1) * grains_len == buf_len);
        }
        new_sample_len += info->order_buffer_lens[i] * info->order_timelens[i]; //calculate new sample size
        sample_position += grains_len_here;
    }
    
    //granular info is created, now granulize
    char* new_sample = calloc(new_sample_len, sizeof(char));

    int next_index_for_writing = 0;
    int index_offset = 0;

    
    int posSpecialGrain = info->order_samples[num_grains - 1];

    for (int i=0; i < num_grains; i++)
    {
        //add grain

        //create new longer grain, scaled as wished by new timelength
        char* buf_new; //new grain stored here
        //int index_offset = info->order_samples[i] * grains_len; //old
        index_offset += grains_len;
        int index = info->order_samples[i] * grains_len;
        int new_index = info->order_samples[i];
        
        int grains_len_here = grains_len;
        if (i == (num_grains - 1))
        { //special case for last grain, this has a different length
            grains_len_here = buf_len - index_offset;
            log_trace("Detected special case");
        }
        log_trace("Write index %i, grains len: %i", index, grains_len_here);
        int res = scale_array_custom_sample_length(buf + index, &buf_new, grains_len_here, info->order_timelens[i], bytes_per_sample);

        //TODO proper error handling
        if (res)
        {

        }
        memcpy(new_sample + next_index_for_writing, buf_new, grains_len_here * info->order_timelens[i]);
        next_index_for_writing += grains_len_here * info->order_timelens[i];
        log_trace("Grain created");
    }

    *buf_out = new_sample;
    *len_out = new_sample_len;

	return info;
}
*/