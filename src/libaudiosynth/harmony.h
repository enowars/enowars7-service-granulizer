#include <stdbool.h>

/*
    Defines a harmony, which consists of different frequencies.
*/
typedef struct harmony {

    float *harmonies;
    int len;

} harmony;

harmony* harmony_create(float *harmonies, int len);

void destroy_harmony(harmony *h);

harmony* gen_octaves(int base_freq, int len);

harmony* gen_fifths(int base_freq, int len, bool round);