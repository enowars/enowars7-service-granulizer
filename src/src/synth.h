#ifndef _SYNTH_H
#define _SYNTH_H

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