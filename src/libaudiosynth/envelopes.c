#include <math.h>


/*
   Generates an linear ADSR Envelope with the given times and writes it to the given buffer.
   The buffer has to have the size samplerate * (attack + decay + sustain + release).

   attack:          attack time in s
   attack_height:   highest point of attack
   decay:           decay time in s
   sustain:         sustain time in s
   sustain_height:  sustain height
   release:         release time in s
   
   Returns number of floats written to buffer
 */
int gen_lin_adsr(float attack, float attack_height, float decay, float sustain, float sustain_height, float release, int samplerate, float* buf) {
  int len_attack  = attack * samplerate;
  int len_decay   = decay * samplerate;
  int len_sustain = sustain * samplerate;
  int len_release = release * samplerate;

  int i;

  //attack:
  int written = 0;

  float ma = attack_height / (len_attack - 1); //degree of incline 
  for ( i=0; i < len_attack; i++ ) {
    buf[written] = ma * i * attack_height;
    written++;
  }
  float highest_point = buf[written - 1];

  //decay:
  float md = (sustain_height - attack_height) / len_decay;
  for ( i=0; i < len_decay; i++ ) {
    buf[written] = highest_point + (i+1) * md;
    written++;
  }

  //sustain:
  for ( i=0; i < len_sustain; i++ ) {
    buf[written] = sustain_height;
    written++;
  }

  //release:
  float mr = (sustain_height) / (len_release-1);
  for ( i=0; i < len_release; i++ ) {
    buf[written] = sustain_height - i * mr;
    written++;
  }

  return written;
}

/*
   Generates an linear ADSR Envelope with the given times and writes it to the given buffer.
   The buffer has to have the size samplerate * (attack + decay + sustain + release).

   attack:          attack time in s
   attack_height:   highest point of attack
   decay:           decay time in s
   sustain:         sustain time in s
   sustain_height:  sustain height, should not be higher than attack_height
   release:         release time in s
   
   Returns number of floats written to buffer
 */
int gen_exp_adsr(float attack, float attack_height, float decay, float sustain, float sustain_height, float release, int samplerate, float* buf) {
  
  int written = 0;

  int len_attack  = attack * samplerate;
  int len_decay   = decay * samplerate;
  int len_sustain = sustain * samplerate;
  int len_release = release * samplerate;

  //attack (exponential)
  //exp: a*e^(x) - 1
  //exponent is scaled to len_attack, so it's always between 0 and 1
  float a = attack_height/(M_E-1);
  for ( int i=0; i < len_attack; i++ ) {
    float value = expf(i/((float)len_attack - 1) ) - 1;
    value = a * value;
    buf[written] = value;
    written++;
  }
  float highest_point = buf[written - 1];
  
  //decay (exponential)
  a = - highest_point/(M_E-1) * sustain_height;
  for ( int i=1; i <= len_decay; i++ ) {
    float value = expf(i/((float)len_decay) ) - 1;
    value = (a * value)  + highest_point;
    buf[written] = value;
    written++;
  }
  
  //sustain (linear):
  for ( int i=0; i < len_sustain; i++ ) {
    buf[written] = sustain_height;
    written++;
  }

  //release (exponential), same principe as decay:
  a = - sustain_height/(M_E-1);
  for ( int i=1; i <= len_release; i++ ) {
    float value = expf(i/((float)len_release) ) - 1;
    value = (a * value)  + sustain_height;
    buf[written] = value;
    written++;
  }

  return written;
}