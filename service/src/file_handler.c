#include "file_handler.h"

#include <string.h>
#include <stdlib.h>
#include "tinywav.h"

/**
 * Reads all data from wav file from the given file_name. 
 */
int read_wav(char* file_name, char** p_data)
{
    TinyWav tw;
    tinywav_open_read(&tw, file_name, TW_INLINE); //SPLIT for each channel in separate buffer
    
    int totalNumSamples = tw.numFramesInHeader;
    
    float* buffer = malloc(totalNumSamples * 4);
    int samplesRead = tinywav_read_f(&tw, buffer, totalNumSamples);
    if (samplesRead == 0) //TODO proper error handling
    {
        return 0;
    }
    tinywav_close_read(&tw);
    
    *p_data = (char*) buffer;
    
    return totalNumSamples;
}

/**
 * Reads all data from given file_name into *p_data
 * @param p_data return pointer to memory
 */
int read_pcm(char* file_name, char** p_data)
{
    //read in complete file
    FILE* f = fopen(file_name, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f); //get size
    fseek(f, 0, SEEK_SET);  //go back to beginning

    char *data = malloc(fsize + 1);
    fread(data, fsize, 1, f);
    fclose(f);

    *p_data = data;

    //p_data[fsize] = 0; //null terminate

    return (int) fsize;
}

int write_pcm(char* file_name, char* p_data, int len)
{
    
    FILE* f = fopen(file_name, "wb");
    if (!f)
    {
        printf("error opening file for writing\n");
        return 1;
    }
    fwrite(p_data, len, 1, f);
    fclose(f);
    return 0;
}

int write_wav(char* file_name, float* p_data, int len)
{
    const int NUM_CHANNELS = 1;
    const int SAMPLE_RATE = 16000;

    TinyWav twWriter;
    tinywav_open_write(&twWriter, NUM_CHANNELS, SAMPLE_RATE, TW_FLOAT32, TW_INLINE, file_name);
    int samplesWritten = tinywav_write_f(&twWriter, p_data, len);
    
    if (samplesWritten == 0) //TODO: proper error checking
    {
        return 0;
    }
    
    tinywav_close_write(&twWriter);
    
    return len;
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
