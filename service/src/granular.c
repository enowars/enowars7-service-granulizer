#include "granular.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "log.c/log.h"

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

static void shuffle(int *array, size_t n)
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

static void shuffle_pointer(void** array, size_t n) {
    srand(time(NULL)); // seed the random number generator

    for (void** i = array + n - 1; i > array; i--) {
        void** j = array + rand() % (i - array + 1);
        void* tmp = *j;
        *j = *i;
        *i = tmp;
    }
}

int generate_random_num(int lower, int upper) {
    int range = upper - lower + 1;
    int random_num = rand() % range;
    int shifted_num = random_num + lower;
    return shifted_num;
}

void reverse(const char* buf, char** buf_out, int buf_len, int block_size) {
    char* reversed = malloc(sizeof(char) * buf_len);
    int start = 0;
    int end = block_size - 1;

    while (end < buf_len) {
        for (int i = end; i >= start; i--) {
            reversed[buf_len - i - 1] = buf[i];
        }
        start += block_size;
        end += block_size;
    }

    if (start < buf_len) {
        for (int i = buf_len - 1; i >= start; i--) {
            reversed[buf_len - i - 1] = buf[i];
        }
    }
    *buf_out = reversed;
}


static int scale_array_custom_sample_length(const char* buf_in, char** buf_out, int buf_in_len, int factor, int bytes_per_sample)
{
    if (factor < 1) {
        return -1;
    }
    if (buf_in_len < bytes_per_sample)
    {
        log_warn("Buf_in_len smaller than bytes_per_sample in granulizing. Is the data not correctly aligned?");
        char *buf = malloc(buf_in_len * sizeof(char));
        memcpy(buf, buf_in, buf_in_len);
        *buf_out = buf;
        return 1;
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
    return factor;
}

/*
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
}*/

void destroy_granular_info(granular_info *g)
{
    if (g)
    {
        free(g->order_samples);
        free(g->order_timelens);
        free(g->order_buffer_lens);
        free(g);
    }
}

void print_granular_info(const granular_info* info)
{
	printf("granular_number_samples = %i\n", info->num_samples);
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
        printf("Grain buf_len_before: %i\n", g->buf_before_len);
        printf("Grain buf_len_after: %i\n", g->buf_after_len);
        
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
        if (g->buf_before_len >= 1)
        {
            printf("Grain buffer_before: [");
            for (int i=0; i < g->buf_before_len - 1; i++)
            {
                printf("%i, ", g->buf_before[i]);
            }
            printf("%i]\n", g->buf_before[g->buf_before_len - 1]);
        }
        if (g->buf_after_len >= 1)
        {
            printf("Grain buffer_after: [");
            for (int i=0; i < g->buf_after_len - 1; i++)
            {
                printf("%i, ", g->buf_after[i]);
            }
            printf("%i]\n", g->buf_after[g->buf_after_len - 1]);
        }
        printf("\n");
    }
}

grain* grain_create()
{
    grain *back = calloc(1, sizeof(grain));
    if (!back)
    {
        return NULL;
    }
    return back;
}

void grain_destroy(grain *g)
{
    if (!g)
    {
        return;
    }

    free(g->buf);
    free(g->buf_before);
    free(g->buf_after);
    free(g);
}

/*
   Generates an exponential ADSR Envelope with the given times and writes it to the given buffer.
   The buffer has to have the size samplerate * (attack + decay + sustain + release).

   attack:          attack time in s
   attack_height:   highest point of attack
   decay:           decay time in s
   sustain:         sustain time in s
   sustain_height:  sustain height, should not be higher than attack_height
   release:         release time in s
   
   Returns number of floats written to buffer
 */
int gen_exp_adsr(float attack, float attack_height, float decay, float sustain, float sustain_height, float release, int samplerate, float* buf) {
  
  int written = 0;

  int len_attack  = attack * samplerate;
  int len_decay   = decay * samplerate;
  int len_sustain = sustain * samplerate;
  int len_release = release * samplerate;

  //attack (exponential)
  //exp: a*e^(x) - 1
  //exponent is scaled to len_attack, so it's always between 0 and 1
  float a = attack_height/(M_E-1);
  for ( int i=0; i < len_attack; i++ ) {
    float value = expf(i/((float)len_attack - 1) ) - 1;
    value = a * value;
    buf[written] = value;
    written++;
  }
  float highest_point = buf[written - 1];
  
  //decay (exponential)
  a = - highest_point/(M_E-1) * sustain_height;
  for ( int i=1; i <= len_decay; i++ ) {
    float value = expf(i/((float)len_decay) ) - 1;
    value = (a * value)  + highest_point;
    buf[written] = value;
    written++;
  }
  
  //sustain (linear):
  for ( int i=0; i < len_sustain; i++ ) {
    buf[written] = sustain_height;
    written++;
  }

  //release (exponential), same principe as decay:
  a = - sustain_height/(M_E-1);
  for ( int i=1; i <= len_release; i++ ) {
    float value = expf(i/((float)len_release) ) - 1;
    value = (a * value)  + sustain_height;
    buf[written] = value;
    written++;
  }

  return written;
}

granular_info* granulize_v2(const char* buf, const int buf_len, char** buf_out, int* len_out, 
    const unsigned int bytes_per_sample, const int samplerate)
{
    log_trace("Starting granulize algorithm with params: buf_len %i, bytes_per_sample %i, samplerate: %i", 
        buf_len, bytes_per_sample, samplerate);
    
    //grains_len has to be minimum bytes_per_sample, choose length so it is
    const int target_grains_per_s = 1;
    int minimum_grain_number = (int) (ceil((double) buf_len / (double) bytes_per_sample)); //minimum possible number of grains when bytes_per_sample are still regarded
    int num_grains = (minimum_grain_number / samplerate * target_grains_per_s);
    if (num_grains > minimum_grain_number)
    {
        num_grains = minimum_grain_number;
    }

    int grain_len = bytes_per_sample; //length of a normal grain, TODO ändern
    log_trace("Num_grains: %i, grain_len: %i, minimum_grain_number: %i", num_grains, grain_len, minimum_grain_number);
    if ((grain_len % bytes_per_sample) != 0)
    {
        log_warn("Grain_len (%i) is not a multiple of bytes_per_sample (%i)! This could lead to weird errors", grain_len, bytes_per_sample);
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
        return NULL;
    }
    info->order_timelens 	= calloc(num_grains, sizeof(int));
    if (!info->order_timelens)
    {
        return NULL;
    }
    info->order_buffer_lens = calloc(num_grains, sizeof(int));
    if (!info->order_buffer_lens)
    {
        return NULL;
    }

    //create grains:
    grain *grains[num_grains];


    const int grain_overlapping_len = grain_len;

    for (int i=0; i < num_grains -1; i++)
    { //even sized grains
        grain *g = grain_create();
        g->orig_pos = i;

        //create actual grain
        g->buf_len = grain_len;
        g->buf = malloc(g->buf_len * sizeof(char));
        int offset_bytes = i * grain_len * sizeof(char);
        void* p_to_cpy_from = buf + offset_bytes;
        memcpy(g->buf, p_to_cpy_from, g->buf_len);
        
        //fill buffer before the actual grain
        int offset_bytes_before = offset_bytes - grain_len;
        int buf_before_len = grain_len; //TODO change, depending on how you want the overlapping
        if (offset_bytes_before < 0)
        {
            offset_bytes_before = offset_bytes;
            buf_before_len = offset_bytes;
        }
        log_trace("Buf before, Len: %i, offset: %i", buf_before_len, offset_bytes_before);
        g->buf_before = malloc(buf_before_len * sizeof(char));
        p_to_cpy_from = buf + offset_bytes_before;
        memcpy(g->buf_before, p_to_cpy_from, buf_before_len);
        g->buf_before_len = buf_before_len;
        
        //fill buffer after the actual grain
        int offset_bytes_after = offset_bytes + grain_len; //start position
        int buf_after_len = grain_len; //TODO change, depending on how you want the overlapping
        if (offset_bytes_after + buf_after_len > buf_len)
        {
            buf_after_len = buf_len - offset_bytes_after;
        }
        log_trace("Buf after, Len: %i, offset: %i", buf_after_len, offset_bytes_after);
        g->buf_after = malloc(buf_after_len * sizeof(char));
        p_to_cpy_from = buf + offset_bytes_after;
        memcpy(g->buf_after, p_to_cpy_from, buf_after_len);
        g->buf_after_len = buf_after_len;

        log_trace("New grain created for index %i", i);
        grains[i] = g;
    }
    //last special shorter grain
    grain *g = grain_create();
    int offset_bytes = (num_grains -1) * grain_len * sizeof(char);
    g->buf_len = buf_len - offset_bytes;
    g->buf = malloc(sizeof(g->buf_len * sizeof(char)));
    g->orig_pos = num_grains-1;
    void* p_to_cpy_from = buf + (num_grains -1) * grain_len;
    memcpy(g->buf, p_to_cpy_from, g->buf_len);
    grains[num_grains - 1] = g;
    //write before & after buffer for this grain
    int offset_bytes_before = offset_bytes - grain_len;
    int buf_before_len = grain_len; //TODO change, depending on how you want the overlapping
    if (offset_bytes_before < 0)
    {
        offset_bytes_before = offset_bytes;
        buf_before_len = offset_bytes;
    }
    log_trace("Buf before, Len: %i, offset: %i", buf_before_len, offset_bytes_before);
    g->buf_before = malloc(buf_before_len * sizeof(char));
    p_to_cpy_from = buf + offset_bytes_before;
    memcpy(g->buf_before, p_to_cpy_from, buf_before_len);
    g->buf_before_len = buf_before_len;
    //fill buffer after the actual grain with 0, since it is the last grain
    g->buf_after_len = 0;
    g->buf_after = NULL;
    //done creating all grains


    //debug grains array:
    for (int i = 0; i < num_grains; i++)
    {
        grain_print_complete(grains[i]);
    }

    //all original grains are now created
    shuffle_pointer(grains, num_grains);
    log_trace("Grains shuffled");

    //apply random timefactor for each grain. Timefactor is maximum MAX_TIMEFACTOR, and could be negative
    for (int i = 0; i < num_grains; i++)
    {
        int timefactor = (rand() % MAX_TIMEFACTOR) + 1;
        int negative = rand() % 2;
        if (negative)
        {
            timefactor = -timefactor;
        }
        grains[i]->used_time_factor = timefactor;
    }


    int new_buf_len = 0;
    //create new grains with new timefactor
    for (int i = 0; i < num_grains; i++)
    {
        grain *g = grains[i];
        //time reversing of grain
        if (g->used_time_factor < 0)
        {
            char *buf_reversed;
            reverse(g->buf, &buf_reversed, g->buf_len, bytes_per_sample);
            free(g->buf);
            g->buf = buf_reversed;

            char *buf_reversed_before; //also reverse data before grain
            reverse(g->buf_before, &buf_reversed_before, g->buf_before_len, bytes_per_sample);
            free(g->buf_before);
            g->buf_before = buf_reversed_before;

            char *buf_reversed_after; //also reverse data after grain
            reverse(g->buf_after, &buf_reversed_after, g->buf_after_len, bytes_per_sample);
            free(g->buf_after);
            g->buf_after = buf_reversed_after;
        }
        //time factor adjusting
        int abs_time_factor = abs(g->used_time_factor);
        char *buf_new = malloc(g->buf_len * abs_time_factor * sizeof(char));
        char *buf_new_before = malloc(g->buf_before_len * abs_time_factor * sizeof(char));
        char *buf_new_after = malloc(g->buf_after_len * abs_time_factor * sizeof(char));
        int res_time_factor = scale_array_custom_sample_length(g->buf, &buf_new, 
            g->buf_len, abs_time_factor, bytes_per_sample);
        int res_time_factor_before = scale_array_custom_sample_length(g->buf_before, &buf_new_before, 
            g->buf_before_len, abs_time_factor, bytes_per_sample);
        int res_time_factor_after = scale_array_custom_sample_length(g->buf_after, &buf_new_after, 
            g->buf_after_len, abs_time_factor, bytes_per_sample);
        
        if (abs_time_factor != res_time_factor)
        {
            log_warn("Scale array did not work due unaligned data. Reset this grains timefactor and length");
            g->used_time_factor = res_time_factor;
            abs_time_factor = res_time_factor;
        }
        free(g->buf);
        free(g->buf_before);
        free(g->buf_after);
        g->buf = buf_new;
        g->buf_len = g->buf_len * abs_time_factor;
        g->buf_before_len = g->buf_before_len * abs_time_factor;
        g->buf_after_len = g->buf_after_len * abs_time_factor;
        
        new_buf_len += g->buf_len;
        new_buf_len += g->buf_before_len;
        new_buf_len += g->buf_after_len;
    }

    int offset = 0;
    //int new_buf_len = 1; //TODO
    char *new_buf = malloc(new_buf_len * sizeof(char));
    //build new grains. Contains original grains and overlapping between
    for (int i = 0; i < num_grains -1; i++)
    {
        log_trace("Next overlapping for grain with index %i", i);
        grain *g_current = grains[i];
        grain *g_next = grains[i + 1];
        //copy current grain
        memcpy(new_buf + offset, g_current->buf, g_current->buf_len);
        offset += g_current->buf_len;

        //create overlay with next grain
        int overlay_buf_len = max(g_current->buf_after_len, g_next->buf_before_len);
        char *overlay_buf = calloc(overlay_buf_len, sizeof(char));
        log_trace("Created overlay buffer with len %i", overlay_buf_len);
        
        for (int j = 0; j < overlay_buf_len; j++)
        {
            double a = -1/(M_E-1);
            double factor_decreasing = exp(j/((double)overlay_buf_len) ) - 1;
            factor_decreasing = (a * factor_decreasing)  + 1;
            double factor_increasing = 1 - factor_decreasing;
            log_trace("Factor decreasing for %i = %lf, increasing = %lf", j, 
                factor_decreasing, factor_increasing);
            
            //fade out for g_current after data
            if (j < g_current->buf_after_len)
            {
                overlay_buf[j] = overlay_buf[j] + 
                    (unsigned char) (factor_decreasing * g_current->buf_after[j]);
                log_trace("Wrote fade out");
            }
            //fade in for g_next before data
            if (j < g_next->buf_before_len)
            {
                overlay_buf[j] = overlay_buf[j] + 
                    (unsigned char) (factor_increasing * g_current->buf_before[j]);
                log_trace("Wrote fade in");
            }
            log_trace("Data in overlay buffer for index %i now: %i (%c)", j, overlay_buf[j], overlay_buf[j]);
        }
        
    }
    //last grain is special, just copy it
    


    for (int i = 0; i < num_grains; i++) //debug
    {
        grain_print_complete(grains[i]);
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