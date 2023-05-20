#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

#include "../src/file_handler.h"

int main()
{
    printf("Testing\n");
    char *f;
    int len = read_wav("example_saw.wav", &f);
    printf("Len: 0x%x\n", len);
    for (int i = 0; i < len; i++)
    {
        printf("%c 0x%x | ", f[i], (unsigned int) f[i]);
    }
    return 0;
}