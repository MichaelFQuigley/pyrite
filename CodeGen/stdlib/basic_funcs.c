#include "basic_funcs.h"

void println(String* str)
{
    printf("%s\n", str->raw_value);
}
