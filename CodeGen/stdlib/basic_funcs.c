#include "basic_funcs.h"

void println(String* str)
{
    printf("%s\n", str->raw_value);
}

void print(String* str)
{
    printf("%s", str->raw_value);
}
