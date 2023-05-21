/**
 * .wav writing / reading inspired from tinywav project, see https://github.com/mhroth/tinywav.
 *
 */

#include "file_handler.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "tinywav.h"
#include "log.c/log.h"



/**
 * Reads all data from wav file from the given file_name.
 * @param p_data return pointer for data
 * @param wavHeader return header which was read from original file, in WavHeader format
 * @returns length of read data in bytes, or -1 for error 
 */
int read_wav(const char* file_name, char** p_data, WavHeader** wavHeader)
{
    log_trace("Read .wav call for file %s", file_name);

    WavHeader *header = calloc(sizeof(WavHeader), 1);

    FILE* f = fopen(file_name, "rb");
    if (!f)
    {
        printf("error opening file\n");
        log_error("error opening file");
        free(header);
        return -1;
    }

    size_t ret = fread(header, sizeof(WavHeader), 1, f); //read header
    
    //check that header has correct format
    if (ret != 1)
    {
        printf("error reading file\n");
        log_error("error reading file");
        free(header);
        fclose(f);
        return -1;
    }
    if (header->ChunkID != htonl(0x52494646)) // "RIFF"
    {
        printf("Error: wav file has wrong format\n");
        log_error("Error: wav file has wrong format");
        free(header);
        fclose(f);
        return -1;
    }
    if (header->Format != htonl(0x57415645)) // "WAVE"
    {
        printf("Error: wav file has wrong format\n");
        log_error("Error: wav file has wrong format");
        free(header);
        fclose(f);
        return -1;
    }
    if (header->Subchunk1ID != htonl(0x666d7420)) // "fmt "
    {
        printf("Error: wav file has wrong format\n");
        log_error("Error: wav file has wrong format");
        free(header);
        fclose(f);
        return -1;
    }

    log_debug("Correct .wav format");

    // skip over any other chunks before the "data" chunk
    bool additionalHeaderDataPresent = false;
    while (header->Subchunk2ID != htonl(0x64617461)) {   // "data"
        log_warn("Additional not data chunk found, ignore");
        fseek(f, 4, SEEK_CUR);
        fread(&header->Subchunk2ID, 4, 1, f);
        additionalHeaderDataPresent = true;
    }

    if (header->Subchunk2ID != htonl(0x64617461))    // "data"
    {
        printf("Error: wav file has wrong format\n");
        log_error("Error: wav file has wrong format");
        free(header);
        fclose(f);
        return -1;
    }
    if (additionalHeaderDataPresent) {
        // read the value of Subchunk2Size, the one populated when reading 'TinyWavHeader' structure is wrong
        fread(&header->Subchunk2Size, 4, 1, f);
        log_debug("Rewriting chunk size, now: %u", header->Subchunk2Size);
    }
    log_debug("Data chunk size: 0x%x", header->Subchunk2Size);

    //header->Subchunk2Size is number of bytes in .wav, read all
    char* data = malloc( header->Subchunk2Size * sizeof(char) );
    
    size_t read = fread(data, sizeof(char), header->Subchunk2Size, f);
    if (read != header->Subchunk2Size)
    {
        printf("Error reading .wav content\n");
        log_error("Error reading .wav content");
        free(header);
        free(data);
        fclose(f);
        return -1;
    }

    free(header);
    fclose(f);
    *p_data = data;
    *wavHeader = header;

    log_info("Success reading .wav file");

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

int write_wav(const char* file_name, const char* p_data, const WavHeader* w_header, const uint32_t len)
{
    log_debug("Start .wav file writing");

    WavHeader* header_cpy = (WavHeader*) w_header;

    //prepare header
    header_cpy->ChunkID = htonl(0x52494646); // "RIFF"
    header_cpy->ChunkSize = 0; // fill this in on file-close
    header_cpy->Format = htonl(0x57415645); // "WAVE"
    header_cpy->Subchunk1ID = htonl(0x666d7420); // "fmt "
    header_cpy->Subchunk1Size = 16; // PCM
    header_cpy->Subchunk2ID = htonl(0x64617461); // "data"
    header_cpy->Subchunk2Size = 0; // fill this in on file-close
    
    FILE* f = fopen(file_name, "wb");
    if (!f)
    {
        log_error("Error opening the file", file_name);

        printf("Error opening the file %s\n", file_name);
        perror("Error opening the file");
        return -1;
    }
    log_info("Opened .wav file successfully");

    //Write header
    size_t written = fwrite(header_cpy, sizeof(WavHeader), 1, f);
    if (written != 1)
    {
        log_error("Error writing header, written bytes %i instead of %i", written, sizeof(WavHeader));
        printf("Error writing .wav file\n");
        perror("Error writing .wav file");
        fclose(f);
        return -1;
    }
    log_debug("Header written successfully");

    //Write data
    written = fwrite(p_data, sizeof(char), len, f);
    if (written != len)
    {
        log_error("Error writing .wav content, written bytes %i instead of %i", written, len * sizeof(char));
        printf("Error writing file\n");
        fclose(f);
        return -1;
    }
    //Update header with content size and chunk size
    fseek(f, 4, SEEK_SET);
    uint32_t chunkSize_len = 36 + len;
    fwrite(&chunkSize_len, sizeof(uint32_t), 1, f);
    
    fseek(f, 40, SEEK_SET);
    fwrite(&len, sizeof(uint32_t), 1, f);

    //Finish
    fclose(f);
    free(header_cpy);

    log_info("Successfully written .wav file");

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
