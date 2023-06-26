#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

#include "granular.h"

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

int read_wav(const char* file_name, char** p_data, WavHeader** wavHeader);

int read_pcm(const char* file_name, char** p_data);

int write_pcm(const char* file_name, const char* p_data, uint32_t len);

int write_wav(const char* file_name, const char* p_data, const WavHeader* w_header, uint32_t len);

bool file_ends_with(const char* str, const char* ending);

bool path_contains_illegal_chars(const char* str);

void granulize_file(const char* file_path, const char* user, granular_info **granular_info);

#endif