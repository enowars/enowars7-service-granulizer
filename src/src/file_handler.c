#include "file_handler.h"

#include <string.h>
#include <stdlib.h>
#include "tinywav.h"

/**
 * Reads all data from wav file from the given file_name. 
 */
char* read_wav(char* file_name)
{ //TODO
    TinyWav tw;
    tinywav_open_read(&tw, file_name, TW_INLINE); //SPLIT for each channel in seperate buffer
    tw.sampFmt = TW_INT16;

    const int NUM_CHANNELS = 1;
    const int BLOCK_SIZE = 65536;

    float* samplePtrs[1];
    
    tinywav_read_f(&tw, samplePtrs, 1);

}

/**
 *
 * Example call: file_ends_with(str_input, ".wav")
 *
 */
bool file_ends_with(char* str, char* ending)
{
    if (!str || !ending) return false;

	char *dot = strrchr(str, '.');
	if (dot)
    {
        return !strcmp(dot, ending);
    }
    return false;
}

bool path_contains_illegal_chars(char* str)
{
    char illegal[] = {'/', '\\'};
    if (strchr(str, illegal[0]) == NULL)
    {
        if (strchr(str, illegal[1]) == NULL)
        {
            return false;
        }
    }
    return true;
}
