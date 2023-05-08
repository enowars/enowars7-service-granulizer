#include <stdlib.h>
#include "harmony.h"

#include <stdbool.h>

/*
    Creates a harmony

    harmonies:  contains all frequencies of the harmony
    len:        number of harmonies in the array

    returns NULL if error occurs
*/
harmony* create_harmony(float *harmonies, int len) {
    harmony* h = calloc( 1, sizeof(harmony) );
    
    h->harmonies = harmonies;
    h->len = len;

    return h;
}

/*
    Destroys the given harmony and frees the space.
*/
void destroy_harmony(harmony *h) {
    free(h->harmonies);
    free(h);
}

/*
    Generates octaves with the given base frequency in the harmony format.
    Returns NULL if error occurs.

 */
harmony* gen_octaves(int base_freq, int len) {
    float *freqs = malloc( len * sizeof(float) );

    freqs[0] = base_freq;
    for (int i=1; i < len; i++) {
        freqs[i] = freqs[i-1] * 2;
    }

    harmony *h = create_harmony(freqs, len);

    return h;
}

/*
 Generates fifths with the given base frequency in the harmony format.
    
    bool round determines, if the frequencies should be rounded down to become integers.
        Set true for more robustness.

    Returns NULL if error occurs
 */
harmony* gen_fifths(int base_freq, int len, bool round) {
    float *freqs = malloc( len * sizeof(float) );

    freqs[0] = base_freq;
    for (int i=1; i < len; i++) {
        freqs[i] = freqs[i-1] * 1.5;
        if (round) {
            freqs[i] = (int) freqs[i];
        }
    }

    harmony *h = create_harmony(freqs, len);

    return h;
}