#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

#include "../src/file_handler.h"

int main()
{
    printf("Testing\n");
    char *p;
    read_wav("../example_saw.wav", &p);
    return 0;
}