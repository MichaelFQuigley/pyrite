#include "basic_funcs.h"

void println(String* str)
{
    printf("%s\n", str->raw_value);
}

void print(String* str)
{
    printf("%s", str->raw_value);
}

gc_base_t* get_back_ptr(void* obj)
{
    return ((gc_base_t**)obj)[0];
}
