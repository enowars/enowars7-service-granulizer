#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdbool.h>

bool file_ends_with(char* str, char* ending);

bool path_contains_illegal_chars(char* str);
#endif