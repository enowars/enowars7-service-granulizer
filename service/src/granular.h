#ifndef _GRANULAR_H
#define _GRANULAR_H

typedef struct granular_info {
	int num_samples;
	int* order_samples;
	int* order_timelens;
} granular_info;

void print_granular_info(granular_info* info);

granular_info* granulize(char* buf, int buf_len, char** buf_out, int* len_out);

int synth_file(char* string, int len);

void get_last_synth_params();

#endif