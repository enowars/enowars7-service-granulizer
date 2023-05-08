#include <math.h>
#include <stdlib.h>

#include "helper.h"

/* Generates a sine sound and writes it to the given buffer. 
  The buffer has to be initialized and the size of samplerate * duration * sizeof(float)
  
  The sine wave has no phase shift and starts at 0.

  freq: frequency of sine in seconds
  duration: duration of sine output in seconds
  amplitude: between 0 and 1
 
   return: 0 if fine, otherwise not 0
 */
int gen_sin(float freq, float duration, float amplitude, float samplerate, float* buf) {
  int len_complete_sin = duration * samplerate;
  int len_one_sin = 1 / freq * samplerate;

  //computates on block of the sinus
  float* sin_buf = malloc( len_one_sin * sizeof(float) );
  if (sin_buf == NULL) return 1; //error
  
  for (int i=0; i < len_one_sin; i++) {
    float radians = 2 * M_PI * i / len_one_sin + 3 * M_PI / 2;
    sin_buf[i] = (sinf(radians) + 1) / 2 * amplitude;
  }

  fill_array_with_pattern(buf, sin_buf, len_complete_sin, len_one_sin);

  free(sin_buf);
  return 0;
}

/* Generates a Sawtooth sound and writes it to the given buffer. 
  The buffer has to be initialized and the size of samplerate * duration * sizeof(float)
 
  freq: frequency of sawtooth in seconds
  duration: duration of sawtooth output in seconds
  amplitude: between 0 and 1
 
   return: 0 if fine, otherwise not 0
 */
int gen_saw(float freq, float duration, float amplitude, float samplerate, float* buf) {
    
    float period_duration = 1/freq;
    
    //creates one sawtooth, which will be copied as many times as needed to the buffer
    int saw_len = (int) (period_duration * samplerate);
    float* saw_buf = malloc(saw_len * sizeof(float));
    if (saw_buf == NULL) return 1; //error
    for (int i=0; i < saw_len; i++) {
      saw_buf[i] = 1.0/saw_len * i * amplitude;
    }
    
    fill_array_with_pattern(buf, saw_buf, duration*samplerate, saw_len);

    free(saw_buf);
    return 0;
}

/* Generates a Triangle sound and writes it to the given buffer. 
  The buffer has to be initialized and the size of samplerate * duration * sizeof(float)
 
  freq: frequency of triangle in seconds
  duration: duration of triangle output in seconds
  amplitude: between 0 and 1
 
   return: 0 if fine, otherwise not 0
 */
int gen_triangle(float freq, float duration, float amplitude, float samplerate, float* buf) {
    
    float period_duration = 1/freq;
    
    //creates one triangle, which will be copied as many times as needed to the buffer
    int tri_len = (int) (period_duration * samplerate);
    int tri_mid = (int) tri_len /2;
    float* tri_buf = malloc(tri_len * sizeof(float));
    if (tri_buf == NULL) return 1; //error
    for (int i=0; i < tri_mid; i++) {
      tri_buf[i] = 1.0/tri_mid * i * amplitude;
    }
    int i2=0;
    tri_buf[tri_mid] = amplitude; //middle point
    for (int i=tri_mid; i < tri_len; i++) {
      tri_buf[i] = tri_buf[tri_mid - i2];
      i2++;
    }
    
    fill_array_with_pattern(buf, tri_buf, duration*samplerate, tri_len);

    free(tri_buf);

    return 0;
}

/*
  Generates a pulse sound and writes it to the given buffer. 
  The buffer has to be initialized and the size of samplerate * duration * sizeof(float)

  freq: frequency in seconds
  pw: pulse width, between 0 and 1. 1 is 100% (always on), 0 is always off
  duration: duration of output in seconds
  amplitude: between 0 and 1  

  return: 0 if fine, otherwise not 0
 */
int gen_pulse(float freq, float pw, float duration, float amplitude, float samplerate, float* buf) {

  float period_duration = 1/freq;

  //generates one pulse circle
  int pulse_len = (int) (period_duration * samplerate);
  int pulse_len_on = (int) (pulse_len * pw);
  float* pulse_buf = calloc(pulse_len, sizeof(float)); //calloc, so everything will be set to 0
  if (pulse_buf == NULL) return 1; //error
  for (int i=0; i < pulse_len_on; i++) { //fill
    pulse_buf[i] = amplitude;
  }

  fill_array_with_pattern(buf, pulse_buf, duration*samplerate, pulse_len);

  free(pulse_buf);

  return 0;
}