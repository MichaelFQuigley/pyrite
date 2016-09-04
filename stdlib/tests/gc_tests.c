#include <stdio.h>

#include "../gc_base.h"


int main(void)
{
   int num_scopes  = 10000;
   int num_objects = 100000;

   gc_init();
   gc_push_scope(); 
   printf("allocating scope\n");
   void* test_ptr = NULL;
    for(int i = 0; i < num_objects; i++)
    {
        test_ptr = gc_malloc(4000000);
   //     ((uint32_t*)test_ptr)[400000] = 1;

        if(i % (num_objects / num_scopes) == 0)
        {
            gc_push_scope(); 
        }
    }

   printf("alloced objects\n");
   sleep(5);

   for(int i = 0; i < num_scopes - 2; i++)
   {
        gc_pop_scope(); 
   }

   gc_mark_and_sweep();
   printf("garbage collected\n");
   sleep(5);

   return 0;
}
