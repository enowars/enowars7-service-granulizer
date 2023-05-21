#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

#include "../src/file_handler.h"
#include "../src/granular.h"

int main(int argc, char **argv)
{
    char* file_name = "example_saw.wav";
    if (argc != 1)
    {
        file_name = argv[1];
    }
    srand(time(NULL)); //create random seed
    printf("Testing\n");
    char *data;
    WavHeader* wavHeader;
    int len = read_wav(file_name, &data, &wavHeader);
    printf("Len: 0x%x\n", len);
    for (int i = 0; i < len; i++)
    {
        printf("0x%x | ", (unsigned char) data[i]);
    }
    printf("WavHeader: \n"),
    printf("Sample Rate %d\n", wavHeader->SampleRate);

    //granulize
    
    char* buf_out;
    int len_out;
    granular_info* g_info = granulize(data, len, &buf_out, &len_out);

    //and now write
    len = write_wav("example_out.wav", buf_out, wavHeader, len_out);
    printf("Written\n");

    return 0;
}