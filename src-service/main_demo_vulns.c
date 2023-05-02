#include <stdio.h>
#include <string.h>

//asm(".globl A");
//asm("A = 0x42");
char input[8] __attribute__((section(".mySection")));
char secret[10] __attribute__((section(".mySection"))) = "FLAG{123}";


int main()
{
    printf("Addr input: 0x%x\n", input);
    printf("Addr secret: 0x%x\n", secret);
    printf("%s", secret);
    fgets(input, 16, stdin);
    printf("You typed in:");
    printf("%s", input);
}