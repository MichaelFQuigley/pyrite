#include <stdio.h>

int main(void)
{
    int i = 1, a = 0, b=2, c =4, d=5,e=6,f=7,g=8;
    void test(void){
    printf("this is a test %d %d %d %d %d %d %d %d \n", i,a,b,c,d,e,f,g);
    }
    test();
    return 0;
}
