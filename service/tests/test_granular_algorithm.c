#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

#include "../src/file_handler.h"
#include "../src/granular.h"
#include "../src/log.c/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void reverse(char* str, int block_size) {
    int len = strlen(str);
    char* reversed = malloc(sizeof(char) * len + 1);
    int start = 0;
    int end = block_size - 1;

    while (end < len) {
        for (int i = end; i >= start; i--) {
            reversed[len - i - 1] = str[i];
        }
        start += block_size;
        end += block_size;
    }

    if (start < len) {
        for (int i = len - 1; i >= start; i--) {
            reversed[len - i - 1] = str[i];
        }
    }
    reversed[len] = '\0';
    strcpy(str, reversed);
    free(reversed);
}

int main() {
    char str[] = "aabbccddee";
    int block_size = 2;
    reverse(str, block_size);
    printf("%s", str);
    return 0;
}

/**
int test_short_sample()
{
    log_info("Starting test_short_sample");

    char buf[] = "AABBCCDDEEF";
    char buf_len = 11;
    char *buf_out;
    int *len_out;
    granulize_v2(buf, buf_len, &buf_out, &len_out, 2, 1);
    log_info("Finished test_short_sample");
    return 0;
}

int main()
{
    log_info("Starting tests");
    //granulize_v2
    
    int res = test_short_sample();
    if (res != 0)
    {
        log_error("Failed: test_short_sample");
    }
    log_info("Finished tests");
}
*/
