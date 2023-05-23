#ifndef _GRANULAR_H
#define _GRANULAR_H

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

void print_granular_info(const granular_info* info);

granular_info* granulize(const char* buf, const int buf_len, char** buf_out, int* len_out, 
    const unsigned int bytes_per_sample, const int samplerate);

#endif