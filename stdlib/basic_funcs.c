#include <assert.h>
#include <string.h>

#include "basic_funcs.h"

void println(void* str_obj) {
  String* str = (String*)str_obj;
  printf("%s\n", str->raw_value);
}

void print(void* str_obj) {
  String* str = (String*)str_obj;
  printf("%s", str->raw_value);
}

void* str(Base* obj) {
  return ((void* (*)())(obj->vtable[TO_STRING_VTABLE_INDEX]))();
}
