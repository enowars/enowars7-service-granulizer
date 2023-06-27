#ifndef _GRANULAR_H
#define _GRANULAR_H

#define MAX_TIMEFACTOR 						((int) 3)

#define TARGET_GRAINS_PER_S_DEFAULT 		((int) 10)
#define GRAIN_TIMEFACTOR_SCALE_DEFAULT 		((int) 2)
#define SAMPLE_VOLUME_DEFAULT				((int) 100)

typedef struct granular_info {
	int num_samples;
	/**
	 * Order of written grains in new granulized data.
	 */
	int* order_samples;
	/**
	 * Timelength of each grain in new granulized data, in granulized data in original order.
	 */
	int* order_timelens;
	/**
	 * Original buffer length of original data, in original order.
	 */
	int* order_buffer_lens;
} granular_info;

typedef struct grain {
	/**
	 * Content of grain. 
	 * Depending on state in program, it either contains the original grain or the modified.
	 */
	char* buf;
	/**
	 * Number of bytes of grain
	 */
	int buf_len;
	
	/**
	 * Content of buffer before grain, for better overlapping effects.
	 */
	char *buf_before;
	int buf_before_len;

	/**
	 * Content of buffer after grain, for better overlapping effects.
	 */
	char *buf_after;
	int buf_after_len;

	/**
	 * Original position of the grain, in units of grains (not bytes).
	 */
	int orig_pos;
	/**
	 * Applied time factor on this grain.
	 */
	int used_time_factor;
} grain;

void destroy_granular_info(granular_info *g);

void print_granular_info(const granular_info* info);

granular_info* granulize(char* buf, const int buf_len, char** buf_out, int* len_out, 
    const unsigned int bytes_per_sample, const int samplerate);

#endif