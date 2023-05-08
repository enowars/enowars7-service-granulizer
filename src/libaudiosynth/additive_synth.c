#include <stdlib.h>

#include "harmony.h"
#include "oscillators.h"
#include "helper.h"

/*
    Generates additive sinus waves with the given harmony
    len: length in seconds
    samplerate: samplerate
    max_amplitude: max_amplitude, like in gen_sin
    harmonies: harmonies for the sinus signal

    Fills the given buffer *buf with the signal.
    The buffer has to have to size len * samplerate

    Returns 0 if it worked, otherwise not 0.
 */
int gen_sound_sin_harmonies(float len, int samplerate, float max_amplitude, harmony *harmonies, float *buf) {
    float* tmp_buf = malloc( len * samplerate * sizeof(float) );

    for (int i=0; i < harmonies->len; i++) {
      float freq = harmonies->harmonies[i];
      int res = gen_sin(freq, len, max_amplitude, samplerate, tmp_buf);
      if (res != 0) return res; //error handling
      //additive sound:
      if (harmonies->len == 1) { //if only one harmony, don't take median
        addFloatArrays(buf, tmp_buf, len * samplerate);
      } else { //'normal' mode
        addFloatArraysMedian(buf, tmp_buf, len * samplerate);
      }
    }

    free(tmp_buf);
    
    return 0;
}