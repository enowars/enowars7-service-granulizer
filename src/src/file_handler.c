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
 * Reads all data from given file_name into p_data
 * @param p_data return pointer to memory
 */
int read_pcm(char* file_name, char* p_data)
{
    //read in complete file
    FILE* f = fopen(file_name, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f); //get size
    fseek(f, 0, SEEK_SET);  //go back to beginning

    *p_data = malloc(fsize + 1);
    fread(p_data, fsize, 1, f);
    fclose(f);

    p_data[fsize] = 0; //null terminate

    return (int) fsize;
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
