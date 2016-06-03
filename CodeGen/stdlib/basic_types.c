#include <stdlib.h>
#include <string.h>
#include "basic_types.h"

//Int
Int* init_Int(int64_t raw_value)
{
    struct Int* new_Int = (struct Int*) malloc(sizeof(Int));
    new_Int->raw_value = raw_value;

    return new_Int;
}

void uninit_Int(Int* int_val)
{
    free(int_val);
}

Int* add_Int(Int* lhs, Int* rhs)
{
    int64_t lhs_raw = lhs->raw_value;
    int64_t rhs_raw = rhs->raw_value;
    
    return init_Int(lhs_raw + rhs_raw);
}

//Float
struct Float* init_Float(double raw_value)
{
    struct Float* new_Float = (struct Float*) malloc(sizeof(Float));
    new_Float->raw_value = raw_value;

    return new_Float;
}

void uninit_Float(struct Float* float_val)
{
    free(float_val);
}

struct Float* add_Float(struct Float* lhs, struct Float* rhs)
{
    double lhs_raw = lhs->raw_value;
    double rhs_raw = rhs->raw_value;
    
    return init_Float(lhs_raw + rhs_raw);
}

//String
struct String* init_String(char* raw_value)
{
    struct String* new_String = (struct String*) malloc(sizeof(String));
    new_String->raw_value = raw_value;

    return new_String;
}

void uninit_String(struct String* str_val)
{
    free(str_val);
}

struct String* add_String(struct String* lhs, struct String* rhs)
{
    char* lhs_raw = lhs->raw_value;
    char* rhs_raw = rhs->raw_value;
   
    char * result = malloc(strlen(lhs_raw) + strlen(rhs_raw) + 1);
    strcpy(result, lhs_raw);
    strcat(result, rhs_raw);
    return init_String(result);
}

//Bool
struct Bool* init_Bool(bool raw_value)
{
    struct Bool* new_Bool = (struct Bool*) malloc(sizeof(Bool));
    new_Bool->raw_value = raw_value;

    return new_Bool;
}

bool rawVal_Bool(struct Bool* this)
{
    return this->raw_value;
}

void uninit_Bool(struct Bool* bool_val)
{
    free(bool_val);
}


/*
int main()
{
    Int* i = init_Int(4);
    Int* j = init_Int(5);
    Int* result = add_Int(i, j);
    printf("%d\n", result->raw_value);
}*/
