#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "gc_base.h"
#include "small_set.h"

//Arithmetic function macros
#define CREATE_NUM_ARITH_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
    void * (FN_NAME ## _ ## STRUCT_TYPE) (void * lhs_obj, va_list * args_rest);

#define CREATE_NUM_ARITH_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)                           \
    void * (FN_NAME ## _ ## STRUCT_TYPE) (void * lhs_obj, va_list * args_rest) {           \
        STRUCT_TYPE * lhs = (STRUCT_TYPE *) lhs_obj;                                       \
        STRUCT_TYPE * rhs = (STRUCT_TYPE *) va_arg(*args_rest, void*);                     \
        return init_ ## STRUCT_TYPE (lhs->raw_value OP rhs->raw_value);                   \
    }
//Compare function macros
#define CREATE_NUM_CMP_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
    void * (FN_NAME ## _ ## STRUCT_TYPE) (void * lhs_obj, va_list * args_rest);

#define CREATE_NUM_CMP_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)                                      \
    void * (FN_NAME ## _ ## STRUCT_TYPE) (void * lhs_obj, va_list * args_rest)  {    \
        STRUCT_TYPE * lhs = (STRUCT_TYPE *) lhs_obj;                                 \
        STRUCT_TYPE * rhs = (STRUCT_TYPE *) va_arg(*args_rest, void*);               \
        return init_Bool( lhs->raw_value OP rhs->raw_value );                       \
    }
//Initialization function macros
#define CREATE_PRIMITIVE_INIT_FN_DECL(STRUCT_TYPE, RAW_TYPE)    \
    void * ( init_ ## STRUCT_TYPE) (RAW_TYPE raw_value);

#define CREATE_PRIMITIVE_INIT_BLOCK(STRUCT_TYPE, RAW_TYPE, OBJ_OUT)                \
        STRUCT_TYPE * OBJ_OUT;                                                 \
        do {                                                                 \
        (OBJ_OUT) = (STRUCT_TYPE *)gc_malloc(sizeof(STRUCT_TYPE));        \
        (OBJ_OUT)->raw_value = raw_value;                \
        (OBJ_OUT)->type_name = # STRUCT_TYPE ;                      \
        (OBJ_OUT) -> funcs = small_set_init(); \
        } while (0);                                                        

#define CREATE_PRIMITIVE_UNINIT_BLOCK(OBJ_IN)                \
    small_set_uninit(((Base*)(OBJ_IN)) ->funcs);

#define BASE_VALS \
    char* type_name;     \
    small_set_t* funcs; 

typedef struct Base
{
    BASE_VALS
} Base;

//Int
typedef struct Int {
    BASE_VALS 

    int64_t raw_value;
} Int;

//Double
typedef struct Float {
    BASE_VALS

    double raw_value;
} Float;

//Bool
typedef struct Bool {
    BASE_VALS

    bool raw_value;
} Bool;

//String
typedef struct String {
    BASE_VALS

    bool raw_is_on_heap;
    char* raw_value;
} String;

//IntRange
typedef struct IntRange {
    BASE_VALS

    Int* curr_val;
    Int* start;
    Int* step;
    Int* end;
} IntRange;

typedef struct List {
    BASE_VALS
    //size is actual size of array
    uint64_t size;
    //capacity represents number of elements are supported by currently allocated array
    size_t capacity;
    void** raw_value;
} List;



CREATE_PRIMITIVE_INIT_FN_DECL(Int, int64_t)

void* String_Int(void* int_val);

void uninit_Int(void* int_val);

CREATE_NUM_ARITH_FN_DECL(Int, int64_t, add, +)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, sub, -)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, mul, *)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, div, /)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, mod, %)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, and, &)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, or, |)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, xor, ^)

CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmplt, <)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmple, <=)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpne, !=)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpgt, >)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpge, >=)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpeq, ==)


CREATE_PRIMITIVE_INIT_FN_DECL(Float, double)

void uninit_Float(void* int_val);

void* String_Float(void* float_val);

CREATE_NUM_ARITH_FN_DECL(Float, double, add, +)
CREATE_NUM_ARITH_FN_DECL(Float, double, sub, -)
CREATE_NUM_ARITH_FN_DECL(Float, double, mul, *)
CREATE_NUM_ARITH_FN_DECL(Float, double, div, /)

CREATE_NUM_CMP_FN_DECL(Float, double, cmplt, <)
CREATE_NUM_CMP_FN_DECL(Float, double, cmple, <=)
CREATE_NUM_CMP_FN_DECL(Float, double, cmpne, !=)
CREATE_NUM_CMP_FN_DECL(Float, double, cmpgt, >)
CREATE_NUM_CMP_FN_DECL(Float, double, cmpge, >=)
CREATE_NUM_CMP_FN_DECL(Float, double, cmpeq, ==)


CREATE_PRIMITIVE_INIT_FN_DECL(String, char*)

void uninit_String(void* int_val);

void* add_String(void* this,  va_list* rhs_obj);

void* String_String(void* this);


CREATE_PRIMITIVE_INIT_FN_DECL(Bool, bool)

void uninit_Bool(void* bool_val);

void* String_Bool(void* bool_val);

bool rawVal_Bool(void* this);


//IntRange
void* init_IntRange(void* start_obj, void* step_obj, void* end_obj);

void uninit_IntRange(void* this);

void* hasNext_IntRange(void* range_obj);

void* next_IntRange(void* range_obj);

void* begin_IntRange(void* range_obj);


//List
void* init_List(uint64_t initial_size);

void uninit_List(void* this);

void set_List(void* this_obj, va_list* args_rest);

void* get_List(void* this_obj, va_list* args_rest);

void* add_List(void* this_obj, va_list* args_rest);

void* String_List(void* this);

void uninit_List(void* arr);

#endif
