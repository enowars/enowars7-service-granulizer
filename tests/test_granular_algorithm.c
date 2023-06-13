#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../src/file_handler.h"
#include "../src/granular.h"
#include "../src/log.c/log.h"


assert_reversed(char *a, char *b, int len)
{
    for (int i = 0; i < len; i++)
    {
        log_trace("Checking a[%i] (%i) == b[%i] (%i)", i, a[i], 
            len - i -1, b[len - i - 1]);
        assert(a[i] == b[len - i - 1]);
    }
}

int test_reverse_small()
{
    log_info("Starting test_reverse_small");
    char str[] = "aabbccddee";
    int str_len = 11;
    int block_size = 2;
    char* buf_out;
    reverse(str, &buf_out, str_len, block_size);
    assert_reversed(str, buf_out, str_len);
    log_info("Finished test_reverse_small");
}


int test_short_sample()
{
    log_info("Starting test_short_sample");

    char buf[] = "AABBCCDDEEFFGGHHIIJJKKLLI";
    char buf_len = 23;
    char *buf_out;
    int *len_out;
    granulize_v2(buf, buf_len, &buf_out, &len_out, 8, 1);

    log_info("Finished test_short_sample");
    return 0;
}

int main()
{
    log_info("Starting tests");
    //granulize_v2
    
    int res;
    res = test_reverse_small();
    if (res != 0)
    {
        log_error("Failed: test_reverse_small");
    }
    res = test_short_sample();
    if (res != 0)
    {
        log_error("Failed: test_short_sample");
    }
    log_info("Finished tests");
}