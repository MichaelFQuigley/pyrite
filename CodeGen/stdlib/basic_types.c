#include <stdlib.h>
#include <string.h>
#include "basic_types.h"
//Int
Int* init_Int(int64_t raw_value)
{
     Int* new_Int = ( Int*) malloc(sizeof(Int));
    new_Int->raw_value = raw_value;

    return new_Int;
}

void uninit_Int(Int* int_val)
{
    free(int_val);
}

CREATE_NUM_ARITH_FN(Int, int64_t, add, +)
CREATE_NUM_ARITH_FN(Int, int64_t, sub, -)
CREATE_NUM_ARITH_FN(Int, int64_t, mul, *)
CREATE_NUM_ARITH_FN(Int, int64_t, div, /)
CREATE_NUM_ARITH_FN(Int, int64_t, mod, %)

CREATE_NUM_CMP_FN(Int, int64_t, cmplt, <)
CREATE_NUM_CMP_FN(Int, int64_t, cmple, <=)
CREATE_NUM_CMP_FN(Int, int64_t, cmpne, !=)
CREATE_NUM_CMP_FN(Int, int64_t, cmpgt, >)
CREATE_NUM_CMP_FN(Int, int64_t, cmpge, >=)
CREATE_NUM_CMP_FN(Int, int64_t, cmpeq, ==)



//Float
 Float* init_Float(double raw_value)
{
     Float* new_Float = ( Float*) malloc(sizeof(Float));
    new_Float->raw_value = raw_value;

    return new_Float;
}

void uninit_Float( Float* float_val)
{
    free(float_val);
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
 String* init_String(char* raw_value)
{
     String* new_String = ( String*) malloc(sizeof(String));
    new_String->raw_value = raw_value;

    return new_String;
}

void uninit_String( String* str_val)
{
    free(str_val);
}

 String* add_String( String* lhs,  String* rhs)
{
    char* lhs_raw = lhs->raw_value;
    char* rhs_raw = rhs->raw_value;
   
    char * result = malloc(strlen(lhs_raw) + strlen(rhs_raw) + 1);
    strcpy(result, lhs_raw);
    strcat(result, rhs_raw);
    return init_String(result);
}


//Bool
Bool* init_Bool(bool raw_value)
{
     Bool* new_Bool = ( Bool*) malloc(sizeof(Bool));
    new_Bool->raw_value = raw_value;

    return new_Bool;
}

bool rawVal_Bool( Bool* this)
{
    return this->raw_value;
}

void uninit_Bool( Bool* bool_val)
{
    free(bool_val);
}

