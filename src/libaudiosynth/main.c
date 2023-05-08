#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "helper.h"
#include "harmony.h"
#include "lib/portsf.h"
#include "envelopes.h"
#include "oscillators.h"
#include "additive_synth.h"

/*
  Maximal amplitude if signal is between 0 and 1.
  TODO find bug why
  It has to be the maximum amplitude when the file is written.
  If it is not used as 'amplitude' in generating elementary signals,
  the audio output written to files will contain -1 where it should be 1.
*/
const float MAX_AMPLITUDE = 0.9999999f;












int main(int argc, char** args) {
    printf("starting...\n");

    if (psf_init()) {
      printf("error psf init.\n");
      return 1;
    }
    printf("Finished initializing\n");
    
    int samplerate = 44100;
    char* file = "output.wav";
    PSF_PROPS props = {
      .srate = samplerate,
      .chans = 1, //mono
      .samptype = PSF_SAMP_32, //32bit
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
    
    //write to file:
    float sound_len = 2; //in s
    float* buf = calloc(samplerate * sound_len, sizeof(float));
    float* buf2 = calloc(samplerate * sound_len, sizeof(float));

    float attack = 1.0f;
    float attack_height = 1.0f;
    float decay = 0.2f;
    float sustain = 0.1f;
    float sustain_height = 0.5f;
    float release = 0.7f;
    
    gen_exp_adsr(attack, attack_height, decay, sustain, sustain_height, release, samplerate, buf2);

    harmony* h = gen_fifths(50, 5, true);
    
    int r = gen_sound_sin_harmonies(sound_len, samplerate, MAX_AMPLITUDE, h, buf);
    
    multiplyFloatArrays(buf, buf2, samplerate * sound_len);

    //Write file
    int res = psf_sndWriteFloatFrames(fd, buf, samplerate * sound_len);
    if (res < 0) {
      printf("Error: %i\n",res);
      return 1;
    }
    printf("Wrote file\n");

    free(buf);
    free(buf2);

    if (psf_sndClose(fd)) {
      printf("Error closing file\n");
      return 1;
    }
    
    
    if (psf_finish()) {
      printf("error finishing psf\n");
      return 1;
    }
    
    return 0;
}
