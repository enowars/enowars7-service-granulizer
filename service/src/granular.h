#ifndef _GRANULAR_H
#define _GRANULAR_H

typedef struct granular_info {
	int num_samples;
	int* order_samples;
	int* order_timelens;
	int grain_len;
} granular_info;

void print_granular_info(const granular_info* info);

granular_info* granulize(const char* buf, const int buf_len, char** buf_out, int* len_out, const unsigned int bytes_per_sample);


#endif