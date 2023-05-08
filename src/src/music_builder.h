#ifndef MUSIC_BUILDER_H
#define MUSIC_BUILDER_H

#include <stdbool.h>

#define MIN_NUM_VOICES 1
#define MAX_NUM_VOICES 8 

#define MIN_NUM_MELODY_NOTES 1
#define MAX_NUM_MELODY_NOTES 16

#define MIN_MELODY_NOTE 1
#define MAX_MELODY_NOTE 23

typedef struct track_info
{
	int num_voices;
	int wave_form;
	int* melody;
	int melody_size;
	float glider;

} track_info;

/*
 * Exposed functions for basic functionality.
 * Sanitizes inputs and all bugs are not intentional.
 */


track_info* track_info_create();

void track_info_destroy(track_info* track);


/**
 * Sets the number of voices for the given track_info.
 * @param num has to be between 1 and 8 (both inclusive)
 * @return true when succeeded, otherwise false
 */
bool set_num_voices(track_info* track, int num);

/**
 *
 * Sets the melody for the given track. 
 * The melody has to have a maximum size of 16 and each element has to be between 1 and 23.
 * (1 is C, 2 D', 3 D and so on, where 1 is the lowest note).
 *
 */
bool set_melody(track_info* track, int* melody, int melody_size);

bool set_wave_form(track_info* track, int wave_form);

bool set_glider(track_info* track, float glider);

float* generate_music(track_info* track);

void filter_music(float music[], float filter[], int strength);

void export_wave(float music[]);

#endif //MUSIC_BUILDER_H