#include <stdio.h>
#include <string.h>

char input[8] __attribute__((section(".d0")));
char secret[8] __attribute__((section(".d1"))) = " FLAG{}\0";
//Sollte bei accident sein

int main()
{
    //Type in 7 chars in order to exploit flag
    printf("Addr input: 0x%p\n",    (void *) input);
    printf("Addr secret: 0x%p\n",   (void *) secret);
    
    char input_tmp[16];
    fgets(input_tmp, 16, stdin);

    int len = strlen(input_tmp);
    if (len > 8)
    {
        printf("Too long!\n");
        return 0;
    } else {
        strncpy(input, input_tmp, 8);
    }
    
    strncpy(secret, "FLAG{1}\0", 8); //place flag

    printf("You typed in:");
    printf("%s", input);
}