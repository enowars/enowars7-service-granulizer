#ifndef _GRANULAR_H
#define _GRANULAR_H

typedef struct granular_info {
	int num_samples;
	int* order_samples;
	int* order_timelens;
} granular_info;

void print_granular_info(granular_info* info);

granular_info* granulize(char* buf, int buf_len);

/**
 * @param in_b64 base64 encoded string for input file
 * @param len len of base64 string
 *
 * @return len written file, -1 if error occurred
*/
int upload_file(char* in_b64, int len);

int synth_file(char* string, int len);

void get_last_synth_params();

#endif