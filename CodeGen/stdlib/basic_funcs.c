#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "basic_funcs.h"

void* str(void* obj)
{
    return lang_try_call(obj, "str");
}

void println(void* str_obj)
{
    String* str = (String*) str_obj;
    printf("%s\n", str->raw_value);
}

void print(void* str_obj)
{
    String* str = (String*) str_obj;
    printf("%s", str->raw_value);
}

void lang_type_assert(void* obj, char* type_name)
{
    if( !lang_type_check(obj, type_name) )
    {
        printf("Runtime Error:\n expected type %s, but received type %s\n",
                type_name,
                ((Base*)obj)->type_name);
        assert(false);
    }
}

bool lang_type_check(void* obj, char* type_name)
{
    return strcmp(((Base*)obj)->type_name, type_name) == 0;
}

bool lang_has_call(void* obj, char* fn_name)
{
    small_set_t* set = (((Base*)obj)->funcs);

    return small_set_get(set, fn_name) != NULL;
}

void* lang_try_call(void* obj, char* fn_name, ...)
{
    va_list args;
    va_start(args, fn_name);
    small_set_t* set = (((Base*)obj)->funcs);

    void* fn = small_set_get(set, fn_name);

    if( !fn )
    {
        printf("No function called %s for type %s\n", 
                fn_name, 
                ((Base*)obj)->type_name);
        assert(false);
    }

    void* result = ((void* (*)(void*, ...))fn)(obj, &args);
    va_end(args);

    return result;
}

void* lang_call(void* obj, char* fn_name, ...)
{
    va_list args;
    va_start(args, fn_name);
    small_set_t* set = (((Base*)obj)->funcs);

    void* fn = small_set_get(set, fn_name);

    void* result = ((void* (*)(void*, ...))fn)(obj, &args);
    va_end(args);

    return result;
}
