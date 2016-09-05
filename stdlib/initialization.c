#include <stdio.h>
#include <stdint.h>

#include "basic_types.h"
#include "gc_base.h"

int initialize_core(void)
{
    int ret_val = 0;
    if( (ret_val = initialize_types()) )
    {
        printf("Error initializing types.\n");
        return ret_val;
    }

    if( (ret_val = gc_init()) )
    {
        printf("Error initializing gc.\n");
        return ret_val;
    }

    return ret_val;
}

int uninitialize_core(void)
{
    int ret_val = 0;

    if( (ret_val = uninitialize_types()) )
    {
        printf("Error uninitializing types.\n");
    }

    return ret_val;
}
