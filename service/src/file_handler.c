#include "file_handler.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinywav.h"
#include "log.c/log.h"

typedef struct WavHeader {
  uint32_t ChunkID;
  uint32_t ChunkSize;
  uint32_t Format;
  uint32_t Subchunk1ID;
  uint32_t Subchunk1Size;
  uint16_t AudioFormat;
  uint16_t NumChannels;
  uint32_t SampleRate;
  uint32_t ByteRate;
  uint16_t BlockAlign;
  uint16_t BitsPerSample;
  uint32_t Subchunk2ID;
  uint32_t Subchunk2Size;
} WavHeader;

/**
 * Reads all data from wav file from the given file_name. 
 */
int read_wav(char* file_name, char** p_data)
{
    log_trace("Read .wav call for file %s.", file_name);

    WavHeader *header = calloc(sizeof(WavHeader), 1);

    FILE* f = fopen(file_name, "rb");
    if (!f)
    {
        printf("error opening file\n");
        log_trace("error opening file");
        free(header);
        return 1;
    }

    size_t ret = fread(header, sizeof(WavHeader), 1, f); //read header
    
    //check that header has correct format
    if (ret <= 0)
    {
        printf("error reading file\n");
        log_trace("error reading file");
        free(header);
        fclose(f);
        return 1;
    }
    if (header->ChunkID != htonl(0x52494646)) // "RIFF"
    {
        printf("Error: wav file has wrong format\n");
        log_trace("Error: wav file has wrong format");
        free(header);
        fclose(f);
        return 1;
    }
    if (header->Format != htonl(0x57415645)) // "WAVE"
    {
        printf("Error: wav file has wrong format\n");
        log_trace("Error: wav file has wrong format");
        free(header);
        fclose(f);
        return 1;
    }
    if (header->Subchunk1ID != htonl(0x666d7420)) // "fmt "
    {
        printf("Error: wav file has wrong format\n");
        log_trace("Error: wav file has wrong format");
        free(header);
        fclose(f);
        return 1;
    }

    log_trace("Correct .wav format");

    // skip over any other chunks before the "data" chunk
    bool additionalHeaderDataPresent = false;
    while (header->Subchunk2ID != htonl(0x64617461)) {   // "data"
        log_trace("Additional not data chunk found, ignore");
        fseek(f, 4, SEEK_CUR);
        fread(header->Subchunk2ID, 4, 1, f);
        additionalHeaderDataPresent = true;
    }
    if (header->Subchunk2ID != htonl(0x64617461))    // "data"
    {
        printf("Error: wav file has wrong format\n");
        log_trace("Error: wav file has wrong format");
        free(header);
        fclose(f);
        return 1;
    }
    if (additionalHeaderDataPresent) {
        // read the value of Subchunk2Size, the one populated when reading 'TinyWavHeader' structure is wrong
        fread(header->Subchunk2Size, 4, 1, f);
        log_trace("Rewriting chunk size, now: %u", header->Subchunk2Size);
    }
    log_trace("Data chunk size: 0x%x", header->Subchunk2Size);

    //header->Subchunk2Size is number of bytes in .wav, read all
    char* data = malloc( header->Subchunk2Size * sizeof(char) );
    
    size_t read = fread(data, sizeof(char), header->Subchunk2Size, f);
    if (read != header->Subchunk2Size)
    {
        printf("Error reading .wav content\n");
        log_trace("Error reading .wav content");
        free(header);
        free(data);
        fclose(f);
        return 1;
    }

    free(header);
    *p_data = data;
    return read;
}

/**
 * Reads all data from given file_name into *p_data
 * @param p_data return pointer to memory
 * @return len of read bytes from file when successful, otherwise -1.
 */
int read_pcm(char* file_name, char** p_data)
{
    //read in complete file
    FILE* f = fopen(file_name, "rb");
    if (!f)
    {
        printf("Error opening the file\n");
        return -1;
    }
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
    //printf("Write file %s with len %i\n", p_data, len);
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
