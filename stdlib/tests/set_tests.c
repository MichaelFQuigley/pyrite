#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "small_set.h"

//NOTE: itoa and reverse taken from 
//https://en.wikibooks.org/wiki/C_Programming/C_Reference/stdlib.h/itoa

 /* reverse:  reverse string s in place */
 void reverse(char s[])
 {
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }

 /* itoa:  convert n to characters in s */
 void itoa(int n, char s[])
 {
     int i, sign;
 
     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }

static char* get_key(int i)
{
   void* buf = malloc(16);
   itoa(i, buf); 

   return buf;
}

static int testfn(int i)
{
    return i;
}

int main(void)
{
    small_set_t* set = small_set_init();
    uint64_t size    = 64;


    for(int i = 0; i < size; i++)
    {
        int* test_int = (int*)malloc(sizeof(int));
        *test_int = i;
        small_set_set(set, get_key(i), test_int);
    }
  
    for(int i = 0; i < size; i++)
    {
        char* key     = get_key(i);
        int* test_int = small_set_get(set, key);

        printf("key = %s\nval = %d\n", key, *test_int);
    }

    assert(size == set->size);
    
    small_set_uninit(set);

    void* fn = testfn;

    ((void (*)(void*, ...))fn)(NULL, 6, 7);

    return 0;
}
