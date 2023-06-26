/**
 * .wav writing / reading inspired from tinywav project, see https://github.com/mhroth/tinywav.
 *
 */

#include "file_handler.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "granular.h"
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
    if (!header)
    {
        log_error("Allocation error");
        return -1;
    }
    *wavHeader = NULL;

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
        printf("Error: wav file has wrong format. Is it a mono channel file?\n");
        log_error("Error: wav file has wrong format");
        free(header);
        fclose(f);
        return -1;
    }

    log_debug("Correct .wav format");

    // skip over any other chunks before the "data" chunk
    bool additionalHeaderDataPresent = false;
    int counter = 0;
    const int MAX_COUNTER = 5;
    while (header->Subchunk2ID != htonl(0x64617461)) {   // "data"
        log_warn("Additional not data chunk found, ignore");
        fseek(f, 4, SEEK_CUR);
        fread(&header->Subchunk2ID, 4, 1, f);
        additionalHeaderDataPresent = true;
        counter++;
        if (counter >= MAX_COUNTER)
        {
            printf("Error: wav file has wrong format. Is it a mono channel file?\n");
            free(header);
            fclose(f);
            return -1;
        }
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
        // read the value of Subchunk2Size, the one populated when reading 'WavHeader' structure is wrong
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
    fclose(f);

    //check for correct byte_depth:
    if (header->BitsPerSample != 8 && header->BitsPerSample != 16 && header->BitsPerSample != 24)
    {
        printf("Error, this file does not have a correct bit depth. Only 8, 16 and 24 bits per sample are allowed\n");
        free(data);
        free(header);
        return -1;
    }
    if (header->BitsPerSample == 24)
    {
        printf("Warning: chosen file has a bit depth of 24, which could lead to sound artifacts.\nReduce the bit depth to 8 or 16 bits per sample for best effects\n");
    }
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
int read_pcm(const char* file_name, char** p_data)
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

    char *data = malloc(fsize);
    int res = fread(data, fsize, 1, f);
    if (res != 1)
    {
        log_warn("Error reading from pcm file");
        free(data);
        fclose(f);
        return -1;
    }
    fclose(f);

    *p_data = data;

    return (int) fsize;
}

int write_pcm(const char* file_name, const char* p_data, uint32_t len)
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

/**
 * @brief 
 * 
 * @param file_name 
 * @param p_data 
 * @param w_header Information about the wave file
 * @param len 
 * @return int 
 */
int write_wav(const char* file_name, const char* p_data, const WavHeader* w_header, uint32_t len)
{
    log_debug("Start .wav file writing for %p with len %i", p_data, len);

    WavHeader* header_cpy = calloc(sizeof(WavHeader), 1);
    memcpy(header_cpy, w_header, sizeof(WavHeader));
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
        free(header_cpy);
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
        free(header_cpy);
        return -1;
    }
    free(header_cpy);
    log_debug("Header written successfully");

    //Write data
    written = fwrite(p_data, len, 1, f);
    if (written != 1)
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

    log_info("Successfully written .wav file");

    return len;
}

/**
 *
 * Example call: file_ends_with(str_input, ".wav")
 *
 */
bool file_ends_with(const char* str, const char* ending)
{
    if (!str || !ending) return false;

	char *dot = strrchr(str, '.');
	if (dot)
    {
        return !strcmp(dot, ending);
    }
    return false;
}

bool path_contains_illegal_chars(const char* str)
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

void granulize_file(const char* file_path, const char* user, granular_info **granular_current_info)
{
    log_trace("File path: %s\n", file_path);
	enum FILE_MODE {
		PCM,
		WAV
	};

	//check that file name is valid
	char *dot = strrchr(file_path, '.');
	if (!dot)
	{
		printf("File ending is missing\n");
		return;
	}
	if (strcmp(dot, ".wav") && strcmp(dot, ".pcm"))
	{
		printf("file has to end with .wav or .pcm\n");
		return;
	}
	log_debug("Correct file ending: %s", dot);

	int len;
	char* p_data;
	WavHeader* w_header = NULL;
	enum FILE_MODE file_mode;

	if (!strcmp(dot, ".wav"))
	{
		file_mode = WAV;
		len = read_wav(file_path, &p_data, &w_header);
		if (len < 1)
		{
			log_error("No data from read_wav, abort granulization");
			printf("Error reading file\n");
			return;
		}
	} else if (!strcmp(dot, ".pcm"))
	{
		file_mode = PCM;
		len = read_pcm(file_path, &p_data);
		if (len < 1)
		{
			log_error("No data from read_pcm, abort granulization");
			printf("Error reading file\n");
			return;
		}
	} else {
		log_error("no pcm or wav file for input, instead: %s", dot);
		printf("Error, wrong file format");
		return;
	}
	log_info("Successfully read data");

	//handle data:
	char *new_sample;
	int new_sample_len;
	int bytes_per_sample;
	int samplerate;
	if (w_header)
	{
		log_trace("Wheader available");
		samplerate = w_header->SampleRate;
		bytes_per_sample = w_header->BitsPerSample / 8;
		log_info("Bytes per sample read: %i", bytes_per_sample);
	} else {
		log_trace("Wheader not available");
		samplerate = 10;
		bytes_per_sample = 1;
	}

	granular_info* info = granulize(p_data, len, &new_sample, &new_sample_len, bytes_per_sample, samplerate);
	if (!info)
	{
		printf("Error granulizing\n");
		if (w_header)
		{
			free(w_header);
			w_header = NULL;
		}
		if (p_data)
		{
			free(p_data);
			p_data = NULL;
		}
		return;
	}
	if (p_data)
	{
		free(p_data);
		p_data = NULL;
	}

	if (*granular_current_info)
	{
		destroy_granular_info(*granular_current_info);
		*granular_current_info = NULL;
	}
	*granular_current_info = info;

	//write data to users folder
    char file_name_complete[128];
	memset(file_name_complete, 0, 128);
	strcpy(file_name_complete, "users/");
	strcat(file_name_complete, user);
	strcat(file_name_complete, "/");
	
	//write file
	if (file_mode == WAV)
	{
		strcat(file_name_complete, "granulized.wav");
		write_wav(file_name_complete, new_sample, w_header, new_sample_len);
	} else if (file_mode == PCM)
	{
		strcat(file_name_complete, "granulized.pcm");
		write_pcm(file_name_complete, new_sample, new_sample_len);
	} else {
		printf("Error, wrong file format");
		log_error("no pcm or wav file for input, instead: %s", dot);
		if (new_sample)
		{
			free(new_sample);
		}
		return;
	}

	if (new_sample)
	{
		free(new_sample);
	}
	if (w_header)
	{
		free(w_header);
		w_header = NULL;
	}

	printf("written to file %s\n", file_name_complete);
	log_info("written to file %s", file_name_complete);
}