#include <stdio.h>
#include <stdbool.h>
#include "basic_types.h"

void println(void* str);
void print(void* str);
gc_base_t* get_back_ptr(void* obj);

void lang_type_assert(void* obj, char* type_name);
bool lang_type_check(void* obj, char* type_name);

bool lang_has_call(void* obj, char* fn_name);
/* lang_try_call
 *
 * tries to call function pointer in object corresponding to fn_name
 *
 * asserts false on failure
 * returns result on success
 *
 */
void* lang_try_call(void* obj, char* fn_name, ...);
