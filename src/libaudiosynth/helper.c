#include <string.h>

#include "harmony.h"

/*
  Fills the given array completely with the pattern.
  Sizes are in elements (1 size <-> 4 bytes)
 */
void fill_array_with_pattern(float* array, float* pattern, int sizeArray, int sizePattern) {
    int whole_len = (int) sizeArray / sizePattern;

    for (int i=0; i < whole_len; i++) {
         memcpy(array + i * sizePattern, pattern, sizePattern * sizeof(float));
    }

    //copy the rest to the output buffer:
    int rest_len = sizeArray - whole_len * sizePattern;
    memcpy(array + whole_len, pattern, rest_len);
} 

/*
  Adds array1 and array2, result is in array1.
 */
void addFloatArrays(float* array1, float* array2, int size) {
    for (int i=0; i < size; i++) {
        array1[i] = array1[i] + array2[i];
    }
}

/*
  Adds array1 and array2, result is in array1. The result will be divided by 2.
 */
void addFloatArraysMedian(float* array1, float* array2, int size) {
    for (int i=0; i < size; i++) {
        array1[i] = (array1[i] + array2[i]) /2;
    }
}

/*
    Adds the constant to each element in the array.
*/
void addFloatArrayConstant(float* array1, int size, float constant) {
    for (int i=0; i < size; i++) {
        array1[i] = array1[i] + constant;
    }
}

/*
    Multiplies each element in array1 and array2, result is in array1.
 */
void multiplyFloatArrays(float* array1, float* array2, int size) {
    for (int i=0; i < size; i++) {
        array1[i] = array1[i] * array2[i];
    }
}

/*
    Multiplies each element in array1 with the given constant.
*/
void multiplyFloatArrayConstant(float* array1, int size, float constant) {
    for (int i=0; i < size; i++) {
        array1[i] = array1[i] * constant;
    }
}