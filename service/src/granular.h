#ifndef _GRANULAR_H
#define _GRANULAR_H

#define MAX_TIMEFACTOR ((int) 3)

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
	 * Content of original grain
	 */
	char* buf;
	/**
	 * Number of bytes of original grain
	 */
	int buf_len;

	/**
	 * Original position of the grain
	 */
	int orig_pos;
	/**
	 * @brief Used time factor on this grain
	 */
	int used_time_factor;
} grain;


void destroy_granular_info(granular_info *g);

void print_granular_info(const granular_info* info);

granular_info* granulize_v2(const char* buf, const int buf_len, char** buf_out, int* len_out, 
    const unsigned int bytes_per_sample, const int samplerate);

granular_info* granulize(const char* buf, const int buf_len, char** buf_out, int* len_out, 
    const unsigned int bytes_per_sample, const int samplerate);

#endif