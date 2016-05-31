#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct sss
{
    int a;
    float b;
    double c;
    int d;
    int e;
};

static int test_c(uint8_t * s)
{
    printf("test %d %d %d %d %d\n", s[0], s[1], s[2], s[3], s[4]);

    return 0;
}

#define SIZE 5
int main(void)
{
    
    uint8_t * s = malloc(sizeof(uint8_t)*SIZE);
    s[0] = 1;
    s[1] = 1;
    s[2] = 1;
    s[3] = 1;
    s[4] = 1;
    test_c(s);
    free(s);
    return 0;
}


