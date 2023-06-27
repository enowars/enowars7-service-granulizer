#ifndef _SHARING_H
#define _SHARING_H

#include "granular.h"

#include <stdbool.h>

char* generate_key(const char* username);

void sharing_allow_call(const char* username);

void sharing_disallow_call(const char* username);

bool sharing_is_allowed(const char* username);

void sharing_use_key_call(const char* own_username, const char* username, const char* entered_key, const char* filename,
    granular_info** current_granular_info);

#endif //_SHARING_H