#include "granular.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <math.h>
#include "log.c/log.h"

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


/**
 * @brief Sets the number of grains per second in a wave file. 
 * This is used by the main function and the 'set option granular_rate' command, 
 * which changes this parameter. 
 */
unsigned int target_grains_per_s = TARGET_GRAINS_PER_S_DEFAULT;

/**
 * @brief Sets the length of the new grains in a wave file. 
 * For PCM files, the timefactor scale will be always the default value.
 * This option for wave files can be applied by the user by using the 'set option time_factor' command.
 */
unsigned int grain_timefactor_scale = GRAIN_TIMEFACTOR_SCALE_DEFAULT;

/**
 * @brief Sets the volume of the new sample.
 * Integer is between 1 (really quiet) and 100 (original loudness).
 */
unsigned int sample_volume = SAMPLE_VOLUME_DEFAULT;


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
    //log_trace("Buf_in_len: %i, Factor: %i, Bytes_per_sample: %i", buf_in_len, factor, bytes_per_sample);

    if (factor < 1) {
        return -1;
    }
    if (buf_in_len < bytes_per_sample)
    {
        //log_warn("Buf_in_len smaller than bytes_per_sample in granulizing. Is the data not correctly aligned?, buf_in_len: %i, bytes_per_sample: %i", buf_in_len, bytes_per_sample);
        char *buf = malloc(buf_in_len * sizeof(char));
        memcpy(buf, buf_in, buf_in_len);
        *buf_out = buf;
        return 1;
    }
    int num_samples = buf_in_len / bytes_per_sample;
    if (buf_in_len % bytes_per_sample != 0)
    {
        //log_warn("Warning buf_in_len %i, bytes_per_sample %i, unaligned?", buf_in_len, bytes_per_sample);
    } else {
        //log_trace("Fine with scaling: buf_in_len %i, bytes_per_sample %i", buf_in_len, bytes_per_sample);
    }
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
    return factor;
}

static granular_info* create_granular_info(int num_grains)
{
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
        free(info);
        return NULL;
    }
    info->order_timelens 	= calloc(num_grains, sizeof(int));
    if (!info->order_timelens)
    {
        free(info->order_samples);
        free(info);
        return NULL;
    }
    info->order_buffer_lens = calloc(num_grains, sizeof(int));
    if (!info->order_buffer_lens)
    {
        free(info->order_timelens);
        free(info->order_samples);
        free(info);
        return NULL;
    }
    return info;
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

/**
 * @brief Function for debugging when working on the algorithm.
 * Is may unused, therefore the attribute is added.
 */
static void __attribute__((unused)) grain_print(grain *g)
{
    if (g)
    {
        if (g->buf)
        {
            printf("Buffer pointer: %p\n", g->buf);
        }
        printf("Grain size: %i\n", g->buf_len);
    }
}

/**
 * @brief Function for debugging when working on the algorithm.
 * Is may unused, therefore the attribute is added.
 */
static void __attribute__((unused)) grain_print_complete(grain *g)
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

static grain* grain_create()
{
    grain *back = calloc(1, sizeof(grain));
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
    if (g->buf_before)
    {
        free(g->buf_before);
        g->buf_before = NULL;
    }
    if (g->buf_after)
    {
        free(g->buf_after);
        g->buf_after = NULL;
    }
    free(g);
}

static grain** create_grains(char *buf, int buf_len, int normal_grain_len, int last_grain_len, int num_grains)
{
    //create array which holds pointers to individual grains
    grain **grains = calloc(num_grains, sizeof(grain*));
    
    for (int i=0; i < num_grains -1; i++)
    { //even sized grains
        grain *g = grain_create();
        g->orig_pos = i;

        //create actual grain
        g->buf_len = normal_grain_len;
        g->buf = malloc(g->buf_len * sizeof(char));
        int offset_bytes = i * normal_grain_len * sizeof(char);
        void* p_to_cpy_from = buf + offset_bytes;
        memcpy(g->buf, p_to_cpy_from, g->buf_len);
        
        //fill buffer before the actual grain
        int offset_bytes_before = offset_bytes - normal_grain_len;
        int buf_before_len = normal_grain_len; //TODO change, depending on how you want the overlapping
        if (offset_bytes_before < 0) //catch out of bounds read
        {
            offset_bytes_before = offset_bytes;
            buf_before_len = offset_bytes;
        }
        if (buf_before_len != 0)
        {
            g->buf_before = malloc(buf_before_len * sizeof(char));
            p_to_cpy_from = buf + offset_bytes_before;
            memcpy(g->buf_before, p_to_cpy_from, buf_before_len);
        } else {
            g->buf_before = NULL;
        }
        g->buf_before_len = buf_before_len;
        //log_trace("Buf before, Len: %i, offset: %i", g->buf_before_len, offset_bytes_before);
        
        //fill buffer after the actual grain
        int offset_bytes_after = offset_bytes + normal_grain_len; //start position
        int buf_after_len = normal_grain_len; //TODO change, depending on how you want the overlapping
        if (offset_bytes_after + buf_after_len > buf_len) //catch out of bounds write
        {
            buf_after_len = buf_len - offset_bytes_after;
        }
        //log_trace("Buf after, Len: %i, offset: %i", buf_after_len, offset_bytes_after);
        g->buf_after = malloc(buf_after_len * sizeof(char));
        p_to_cpy_from = buf + offset_bytes_after;
        memcpy(g->buf_after, p_to_cpy_from, buf_after_len);
        g->buf_after_len = buf_after_len;

        //log_trace("New grain created for index %i", i);
        grains[i] = g;
    }
    log_trace("Create special last grain");
    //last special shorter grain
    grain *g = grain_create();
    g->buf_len = last_grain_len;
    g->buf = malloc(g->buf_len * sizeof(char));
    g->orig_pos = num_grains-1;
    int offset = (num_grains -1) * normal_grain_len;
    void* p_to_cpy_from = buf + offset;
    log_trace("Buf, Len: %i, offset: %i", g->buf_len, offset);
    memcpy(g->buf, p_to_cpy_from, g->buf_len);

    //write before buffer for this special grain
    int offset_bytes_before = (num_grains -1) * normal_grain_len - normal_grain_len;
    int buf_before_len = normal_grain_len; //TODO maybe change, depending on how you want the overlapping
    if (offset_bytes_before < 0)
    {
        log_warn("Offset bytes before < 0, is the provided data too short?");
        //buf_before_len = offset_bytes;
    }
    log_trace("Buf before, Len: %i, offset: %i", buf_before_len, offset_bytes_before);
    g->buf_before = malloc(buf_before_len * sizeof(char));
    g->buf_before_len = buf_before_len;
    p_to_cpy_from = buf + offset_bytes_before;
    memcpy(g->buf_before, p_to_cpy_from, g->buf_before_len);
    
    //fill buffer after the actual grain with 0, since it is the last grain
    g->buf_after_len = 0;
    g->buf_after = NULL;

    grains[num_grains - 1] = g;
    //done creating all grains

    return grains;
}

static void apply_random_timefactors(grain** grains, int num_grains)
{
    for (int i = 0; i < num_grains; i++)
    {
        grains[i]->used_time_factor = grain_timefactor_scale;
    }

    /*
     * This code can be used in future if the granulizing algorithm is ready for different timelengths.
    */
    //apply random timefactor for each grain. Timefactor is maximum MAX_TIMEFACTOR, and could be negative
    /*
    for (int i = 0; i < num_grains; i++)
    {
        //int timefactor = (rand() % MAX_TIMEFACTOR) + 1;
        int timefactor = 2; //TODO change
        int negative = rand() % 2; //TODO CURRENTLY NOT WORKING!
        negative = 0;
        if (negative)
        {
            timefactor = -timefactor;
        }
        grains[i]->used_time_factor = timefactor;
    }*/
}

static void apply_new_volume(char *new_buf, int new_buf_len, int bytes_per_sample)
{
    double factor_volume = sample_volume / 100.0;
    log_debug("Apply volume factor of: %lf", factor_volume);
    for (int j=0; j < new_buf_len; j += bytes_per_sample)
    {
        if (bytes_per_sample == 1)
        {
            int8_t x = ((new_buf[j+0] << 0) & 0xFF);
            x = (uint8_t) (x * factor_volume);
            new_buf[j+0] = (uint8_t) (x & 0x00FF);
        } else if (bytes_per_sample == 2) {
            int16_t x = ((new_buf[j+1] << 8) & 0xFF00) | 
                        ((new_buf[j+0] << 0) & 0xFF);
            x = (uint16_t) (x * factor_volume);
            new_buf[j+1] = (uint8_t) ((x & 0xFF00) >> 8);
            new_buf[j+0] = (uint8_t) (x & 0x00FF);
        } else if (bytes_per_sample == 3) {
            int32_t x = ((new_buf[j+2] << 16) & 0x00FF0000) | 
                        ((new_buf[j+1] << 8)  & 0x0000FF00) | 
                        ((new_buf[j+0] << 0)  & 0x000000FF);
            x = (uint32_t) (x * factor_volume);
            new_buf[j+2] = (uint8_t) ((x & 0xFF0000) >> 16);
            new_buf[j+1] = (uint8_t) ((x & 0xFF00) >> 8);
            new_buf[j+0] = (uint8_t) (x & 0x00FF);
        }
    }
}

static int change_grains(grain** grains, int num_grains, int bytes_per_sample, int overlay_len)
{
    int new_buf_len = 0; //length of new buffer, with correct applied timelengths
    //create new grains with new timefactor
    for (int i = 0; i < num_grains; i++)
    {
        grain *g = grains[i];
        //log_trace("Grain [%i], buf_len %i, buf_len_before %i, buf_len_after %i, timescale: %i", i, 
        //    g->buf_len, g->buf_before_len, g->buf_after_len, g->used_time_factor);
        //time reversing of grain
        if (g->used_time_factor < 0)
        {
            if (g->buf_len != 0)
            {
                char *buf_reversed;
                reverse(g->buf, &buf_reversed, g->buf_len, bytes_per_sample);
                free(g->buf);
                g->buf = buf_reversed;
            }
            if (g->buf_before_len != 0)
            {
                char *buf_reversed_before; //also reverse data before grain
                reverse(g->buf_before, &buf_reversed_before, g->buf_before_len, bytes_per_sample);
                free(g->buf_before);
                g->buf_before = buf_reversed_before;
            }
            if (g->buf_after_len != 0)
            {
                char *buf_reversed_after; //also reverse data after grain
                reverse(g->buf_after, &buf_reversed_after, g->buf_after_len, bytes_per_sample);
                free(g->buf_after);
                g->buf_after = buf_reversed_after;
            }
        }

        //time factor adjusting, if there is a buffer to work on
        int abs_time_factor = abs(g->used_time_factor);

        char *buf_new;
        char *buf_new_before;
        char *buf_new_after;
        if (g->buf_len != 0)
        {
            int res_time_factor = scale_array_custom_sample_length(g->buf, 
            &buf_new, 
            g->buf_len, abs_time_factor, bytes_per_sample);
            free(g->buf);
            g->buf = buf_new;
            g->buf_len = g->buf_len * abs_time_factor;
            new_buf_len += g->buf_len;
            if (abs_time_factor != res_time_factor)
            {
                log_warn("Scale array did not work due unaligned data. Reset this grains timefactor and length");
                g->used_time_factor = res_time_factor;
                abs_time_factor = res_time_factor;
            }
        }
        
        if (g->buf_before_len != 0)
        {
            scale_array_custom_sample_length(g->buf_before, 
            &buf_new_before, 
            g->buf_before_len, abs_time_factor, bytes_per_sample);
            free(g->buf_before);
            g->buf_before = buf_new_before;
            g->buf_before_len = g->buf_before_len * abs_time_factor;
            //new_buf_len += g->buf_before_len;
        }
        if (g->buf_after_len != 0)
        {
            scale_array_custom_sample_length(g->buf_after, 
            &buf_new_after, 
            g->buf_after_len, abs_time_factor, bytes_per_sample);
            free(g->buf_after);
            g->buf_after = buf_new_after;
            g->buf_after_len = g->buf_after_len * abs_time_factor;
            //new_buf_len += g->buf_after_len;
        }
    }
    new_buf_len += (num_grains - 1) * overlay_len;

    log_debug("New buf len: %i", new_buf_len);
    return new_buf_len;
}

static char* build_new_sample(grain** grains, int num_grains, 
    int bytes_per_sample, int new_buf_len, granular_info* info)
{
    int offset = 0;
    char *new_buf = calloc(new_buf_len, sizeof(char));
    
    //build new grains. Contains original grains and overlapping in between
    for (int i = 0; i < num_grains -1; i++)
    {
        //log_trace("Next overlapping for grain with index %i", i);
        grain *g_current = grains[i];
        grain *g_next = grains[i + 1];

        //copy actual grain
        memcpy(new_buf + offset, g_current->buf, g_current->buf_len);
        offset += g_current->buf_len;

        //update granular info
        info->order_samples[i] = g_current->orig_pos;
        info->order_timelens[i] = g_current->used_time_factor;
        info->order_buffer_lens[i] = g_current->buf_len;

        //create overlay with next grain
        //int overlay_buf_len = max(g_current->buf_after_len, g_next->buf_before_len);
        int overlay_buf_len = grains[0]->buf_len;

        char *overlay_buf = calloc(overlay_buf_len, sizeof(char)); //use calloc for init with 0
        //log_trace("Created overlay buffer with len %i, buf_after_len %i, buf_before_len %i", 
        //    overlay_buf_len, g_current->buf_after_len, g_next->buf_before_len);

        //fade in + fade out
        for (int j = 0; j < overlay_buf_len; j += bytes_per_sample)
        {
            double a = -1/(M_E-1);
            double factor_decreasing = exp(j/((double)overlay_buf_len) ) - 1;
            factor_decreasing = (a * factor_decreasing) + 1;
            double factor_increasing = 1 - factor_decreasing;
            
            if (factor_decreasing >= 0.99) { //normalizing for anti clipping
                factor_decreasing = 1;
            } else if (factor_decreasing <= 0.01) {
                factor_decreasing = 0;
            }
            if (factor_increasing >= 0.99) {
                factor_increasing = 1;
            } else if (factor_increasing <= 0.01) {
                factor_increasing = 0;
            }

            //fade out for g_current after data
            if (g_current)
            {
                if (j + (bytes_per_sample - 1) < g_current->buf_after_len)
                {
                    if (bytes_per_sample == 1)
                    {
                        int8_t x = ((g_current->buf_after[j+0] << 0) & 0xFF);
                        x = (uint8_t) (x * factor_decreasing);
                        overlay_buf[j+0] = (uint8_t) (x & 0x00FF);
                    } else if (bytes_per_sample == 2) {
                        int16_t x = ((g_current->buf_after[j+1] << 8) & 0xFF00) | ((g_current->buf_after[j+0] << 0) & 0xFF);
                        x = (uint16_t) (x * factor_decreasing);
                        overlay_buf[j+1] = (uint8_t) ((x & 0xFF00) >> 8);
                        overlay_buf[j+0] = (uint8_t) (x & 0x00FF);
                    } else if (bytes_per_sample == 3) {
                        int32_t x = ((g_current->buf_after[j+2] << 16) & 0x00FF0000) | 
                                    ((g_current->buf_after[j+1] << 8)  & 0x0000FF00) | 
                                    ((g_current->buf_after[j+0] << 0)  & 0x000000FF);
                        x = (uint32_t) (x * factor_decreasing);
                        overlay_buf[j+2] = (uint8_t) ((x & 0xFF0000) >> 16);
                        overlay_buf[j+1] = (uint8_t) ((x & 0xFF00) >> 8);
                        overlay_buf[j+0] = (uint8_t) (x & 0x00FF);
                    }
                }
            }
            if (g_next)
            {
                if (j + (bytes_per_sample - 1) < g_next->buf_before_len) //fade in for g_next before data
                {
                    if (bytes_per_sample == 1)
                    {
                        int8_t x       = ((g_next->buf_before[j+0] << 0) & 0xFF);
                        uint8_t orig    = ((overlay_buf[j+0] << 0) & 0xFF);
                        x = (uint8_t) (x * factor_increasing);
                        x += orig;
                        overlay_buf[j+0] = (x & 0x00FF);
                    } else if (bytes_per_sample == 2) {
                        int16_t x       = ((g_next->buf_before[j+1] << 8) & 0xFF00) | ((g_next->buf_before[j+0] << 0) & 0xFF);
                        uint16_t orig    = ((overlay_buf[j+1] << 8) & 0xFF00) | ((overlay_buf[j+0] << 0) & 0xFF);
                        x = (uint16_t) (x * factor_increasing);
                        x += orig;
                        overlay_buf[j+1] = ((x & 0xFF00) >> 8);
                        overlay_buf[j+0] = (x & 0x00FF);
                    } else if (bytes_per_sample == 3) {
                        int32_t x       = 
                            ((g_next->buf_before[j+2] << 16) & 0xFF0000) |
                            ((g_next->buf_before[j+1] << 8) & 0xFF00) | 
                            ((g_next->buf_before[j+0] << 0) & 0xFF);
                        uint32_t orig    = 
                            ((overlay_buf[j+2] << 16) & 0xFF0000) |
                            ((overlay_buf[j+1] << 8) & 0xFF00) | 
                            ((overlay_buf[j+0] << 0) & 0xFF);
                        x = (uint32_t) (x * factor_increasing);
                        x += orig;
                        overlay_buf[j+2] = ((x & 0xFF0000) >> 16);
                        overlay_buf[j+1] = ((x & 0xFF00) >> 8);
                        overlay_buf[j+0] = (x & 0x00FF);
                    }

                } else {
                    //log_warn("Out of bounds for in fading");
                }
            }
        }
        memcpy(new_buf + offset, overlay_buf, overlay_buf_len);
        free(overlay_buf);
        offset += overlay_buf_len;
    }
    //last grain is special, just copy it
    memcpy(new_buf + offset, grains[num_grains - 1]->buf, grains[num_grains - 1]->buf_len);
    //update granular info
    info->order_samples[num_grains - 1]         = grains[num_grains - 1]->orig_pos;
    info->order_timelens[num_grains - 1]        = grains[num_grains - 1]->used_time_factor;
    info->order_buffer_lens[num_grains - 1]     = grains[num_grains - 1]->buf_len;
    //fade this one out
        //TODO
    return new_buf;
}

granular_info* granulize(char* buf, const int buf_len, char** buf_out, int* len_out, 
    const unsigned int bytes_per_sample, const int samplerate)
{
    log_trace("Starting granulize v2 algorithm with params: buf_len %i, bytes_per_sample %i, samplerate: %i", 
        buf_len, bytes_per_sample, samplerate);
    
    int num_grains = 0;
    int normal_grain_len = 0;
    int last_grain_len = 0;
    if (bytes_per_sample != 1)
    { //more difficult case for .wav data
        //grains_len has to be minimum bytes_per_sample, choose length so it is

        num_grains = (int) ((double)buf_len / (double) samplerate * (double)target_grains_per_s);
        if (buf_len % bytes_per_sample != 0)
        {
            log_warn("Invalid data for granulizing provided");
            printf("Error, unaligned data for granulize provided\n");
            return NULL;
        }
        //units in bytes
        normal_grain_len = ((int) ((double) buf_len / 2 / (double) num_grains)) * 2;
        last_grain_len = buf_len - (normal_grain_len * (num_grains - 1));
        
        log_trace("num_grains: %i, normal_grain_len: %i, last_grain_len: %i", num_grains, normal_grain_len, last_grain_len);
        
        if (num_grains <= 1)
        {
            return NULL;
        }
    } else { //bytes_per_sample = 1, easy case
        log_trace("Detected .pcm granulize data on byte level");
        //resetting this option, which is not valid for pcm files
        grain_timefactor_scale = GRAIN_TIMEFACTOR_SCALE_DEFAULT;
        num_grains = buf_len;
        normal_grain_len = 1;
        last_grain_len = 1;
    }

    granular_info *info = create_granular_info(num_grains);

    grain **grains = create_grains(buf, buf_len, normal_grain_len, last_grain_len, num_grains);
    log_debug("Created grains");

    //all original grains are now created
    shuffle_pointer((void**)grains, num_grains);
    log_debug("Grains shuffled");

    //apply set timefactor for wav, otherwise default value
    //TODO implement that
    apply_random_timefactors(grains, num_grains);
    log_debug("Applied new timefactors for grains");

    int new_buf_len = change_grains(grains, num_grains, bytes_per_sample, 
        normal_grain_len * grains[0]->used_time_factor); //this assumes that the timefactor is everywhere the same
    log_debug("Changed original grains");

    char *new_buf = build_new_sample(grains, num_grains, bytes_per_sample, new_buf_len, info);
    log_debug("Done building new sample");

    //apply new volume
    if (sample_volume != 100)
    {
        apply_new_volume(new_buf, new_buf_len, bytes_per_sample);
    }

    //cleanup
    for (int i = 0; i < num_grains; i++)
    {
        grain_destroy(grains[i]);
    }
    free(grains);

    //return
    *len_out = new_buf_len;
    *buf_out = new_buf;
    return info;
}