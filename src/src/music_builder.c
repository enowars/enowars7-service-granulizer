#include "music_builder.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../libaudiosynth/lib/portsf.h"

/*
 * Exposed functions for basic functionality.
 * Sanitizes inputs and all bugs are not intentional.
 */

track_info* track_info_create()
{
    track_info* back = malloc(sizeof(track_info));
    if (back)
    {
        back->num_voices = 0;
        back->wave_form = 0;
        back->melody = 0;
        back->melody_size = 0;
        back->glider= 0;
    } else {
        return 0;
    }
    
    return back;
}
void track_info_destroy(track_info* track)
{
    free(track);
}

/**
 * Sets the number of voices for the given track_info.
 * @param num has to be between 1 and 8 (both inclusive)
 * @return true when succeeded, otherwise false
 */
bool set_num_voices(track_info* track, int num)
{
	if (num >= MIN_NUM_VOICES && num <= MAX_NUM_VOICES)
	{
		track->num_voices = num;
		return true;
	} else {
		printf("Error, set num voices only supports voices between 1 and 8.");
		return false;
	}
}

/**
 *
 * Sets the melody for the given track. 
 * The melody has to have a maximum size of 16 and each element has to be between 1 and 23.
 * (1 is C, 2 D', 3 D and so on, where 1 is the lowest note).
 *
 */
bool set_melody(track_info* track, int* melody, int melody_size)
{
	if (melody_size >= MIN_NUM_MELODY_NOTES && melody_size <= MAX_NUM_MELODY_NOTES)
	{

		//check that each note in melody is in correct range
		for (int i = 0; i < melody_size; i++)
		{
			if ( !(melody[i] >= MIN_MELODY_NOTE && melody[i] >= MAX_MELODY_NOTE) )
			{
				printf("Note in melody is not in the correct range\n");
				return false;
			}
		}

		//all input is correct at this point
		track->melody = melody;
		track->melody_size = melody_size;
		return true;
	
	} else {
		return false;
		printf("Number of notes in melody is not correct\n");
	}

}

bool set_wave_form(track_info* track, int wave_form)
{
	track->wave_form = wave_form;
}

bool set_glider(track_info* track, float glider)
{
	track->glider = glider;
}

float* generate_music(track_info* track)
{

}

void filter_music(float music[], float filter[], int strength)
{

}

void export_wave(float music[])
{
    int samplerate = 44100;
    int sound_len = 1;

    char* file = "output.wav";
    PSF_PROPS props = {
      .srate = samplerate,
      .chans = 1, //mono
      .samptype = PSF_SAMP_16, //16bit
      .format = PSF_STDWAVE,
      .chformat = MC_MONO
    };
    int clip_floats = 1;
    int minheader = 0;
    int mode = PSF_CREATE_RDWR;

    int fd = psf_sndCreate(file, &props, clip_floats, minheader, mode);
    if (fd < 0) {
      printf("Error creating soundfile\n");
      return 1;
    }
    printf("Soundfile created: %s\n", file);
    
    //Write file
    int res = psf_sndWriteFloatFrames(fd, music, samplerate * sound_len);
    if (res < 0) {
      printf("Error: %i\n",res);
      return 1;
    }
    printf("Wrote file\n");
}