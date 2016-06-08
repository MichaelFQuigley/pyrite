#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "basic_types.h"

//TODO change how the String_* methods allocate memory

//Int
CREATE_PRIMITIVE_INIT_FN(Int, int64_t)

void uninit_Int(Int* int_val)
{
    free(int_val);
}

void String_Int(Int* int_val, String* result)
{
    size_t buffer_size = 32;
    char* buffer       = (char*) malloc(buffer_size);
    //TODO error checking for sprintf, maybe
    snprintf(buffer, buffer_size, "%ld", int_val->raw_value);

    result->raw_value      = buffer;
    result->raw_is_on_heap = true;
}

CREATE_NUM_ARITH_FN(Int, int64_t, add, +)
CREATE_NUM_ARITH_FN(Int, int64_t, sub, -)
CREATE_NUM_ARITH_FN(Int, int64_t, mul, *)
CREATE_NUM_ARITH_FN(Int, int64_t, div, /)
CREATE_NUM_ARITH_FN(Int, int64_t, mod, %)
CREATE_NUM_ARITH_FN(Int, int64_t, and, &)
CREATE_NUM_ARITH_FN(Int, int64_t, or, |)
CREATE_NUM_ARITH_FN(Int, int64_t, xor, ^)

CREATE_NUM_CMP_FN(Int, int64_t, cmplt, <)
CREATE_NUM_CMP_FN(Int, int64_t, cmple, <=)
CREATE_NUM_CMP_FN(Int, int64_t, cmpne, !=)
CREATE_NUM_CMP_FN(Int, int64_t, cmpgt, >)
CREATE_NUM_CMP_FN(Int, int64_t, cmpge, >=)
CREATE_NUM_CMP_FN(Int, int64_t, cmpeq, ==)



//Float
CREATE_PRIMITIVE_INIT_FN(Float, double)

void uninit_Float( Float* float_val)
{
    free(float_val);
}

void String_Float(Float* float_val, String* result)
{
    size_t buffer_size = 32;
    char* buffer       = (char*) malloc(buffer_size);
    //TODO error checking for sprintf, maybe
    snprintf(buffer, buffer_size, "%f", float_val->raw_value);
    result->raw_value      = buffer;
    result->raw_is_on_heap = true;
}

CREATE_NUM_ARITH_FN(Float, double, add, +)
CREATE_NUM_ARITH_FN(Float, double, sub, -)
CREATE_NUM_ARITH_FN(Float, double, mul, *)
CREATE_NUM_ARITH_FN(Float, double, div, /)

CREATE_NUM_CMP_FN(Float, double, cmplt, <)
CREATE_NUM_CMP_FN(Float, double, cmple, <=)
CREATE_NUM_CMP_FN(Float, double, cmpne, !=)
CREATE_NUM_CMP_FN(Float, double, cmpgt, >)
CREATE_NUM_CMP_FN(Float, double, cmpge, >=)
CREATE_NUM_CMP_FN(Float, double, cmpeq, ==)


//String
CREATE_PRIMITIVE_INIT_FN(String, char*)

void uninit_String( String* str_val)
{
    if( str_val->raw_is_on_heap )
    {
        free(str_val->raw_value);
    }
    free(str_val);
}

void add_String( String* lhs,  String* rhs, String* result)
{
    char* lhs_raw = lhs->raw_value;
    char* rhs_raw = rhs->raw_value;
   
    char * result_buf = malloc(strlen(lhs_raw) + strlen(rhs_raw) + 1);
    strcpy(result_buf, lhs_raw);
    strcat(result_buf, rhs_raw);
    result->raw_value      = result_buf;
    result->raw_is_on_heap = true;
}


//Bool
CREATE_PRIMITIVE_INIT_FN(Bool, bool)

void String_Bool(Bool* bool_val, String* result)
{
    size_t buffer_size = 5;
    char* buffer       = (char*) calloc(1, buffer_size);
    //TODO error checking for sprintf, maybe
    strcpy(buffer, (bool_val->raw_value) ? "true" : "false");
    result->raw_value = buffer;
    result->raw_is_on_heap = true;
}

bool rawVal_Bool(Bool* this)
{
    return this->raw_value;
}

void uninit_Bool( Bool* bool_val)
{
    free(bool_val);
}

