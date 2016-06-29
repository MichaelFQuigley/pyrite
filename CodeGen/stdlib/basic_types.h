#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "gc_base.h"

//Arithmetic function macros
#define CREATE_NUM_ARITH_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
    STRUCT_TYPE * (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs);

#define CREATE_NUM_ARITH_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)                           \
    STRUCT_TYPE * (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs) {  \
        return init_ ## STRUCT_TYPE (lhs->raw_value OP rhs->raw_value);                   \
    }
//Compare function macros
#define CREATE_NUM_CMP_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
    Bool * (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs);

#define CREATE_NUM_CMP_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)                                      \
    Bool * (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs) {    \
        return init_Bool( lhs->raw_value OP rhs->raw_value );                        \
    }
//Initialization function macros
#define CREATE_PRIMITIVE_INIT_FN_DECL(STRUCT_TYPE, RAW_TYPE)    \
    STRUCT_TYPE * ( init_ ## STRUCT_TYPE) (RAW_TYPE raw_value);

#define CREATE_PRIMITIVE_INIT_FN(STRUCT_TYPE, RAW_TYPE)                      \
    STRUCT_TYPE * ( init_ ## STRUCT_TYPE) (RAW_TYPE raw_value) {  \
        gc_base_t* val = (gc_base_t *)gc_malloc(sizeof(STRUCT_TYPE));    \
        ((STRUCT_TYPE *)val->raw_obj)->raw_value = raw_value;                \
        ((STRUCT_TYPE *)val->raw_obj)->back_ptr  = val;                      \
        ((STRUCT_TYPE *)val->raw_obj)->type_name = # STRUCT_TYPE ;                      \
        return (STRUCT_TYPE*) (val->raw_obj);                                \
    }

#define BASE_VALS \
    gc_base_t* back_ptr; \
    char* type_name;

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

String* String_Int(Int* int_val);

void uninit_Int(Int* int_val);

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

void uninit_Float(Float* int_val);

String* String_Float(Float* float_val);

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

void uninit_String(String* int_val);

String* add_String(String* lhs, String* rhs);

CREATE_PRIMITIVE_INIT_FN_DECL(Bool, bool)

void uninit_Bool(Bool* int_val);

String* String_Bool(Bool* bool_val);

bool rawVal_Bool(Bool* this);


IntRange* init_IntRange(Int* start, Int* step, Int* end);

Bool* hasNext_IntRange(IntRange* range);

Int* next_IntRange(IntRange* range);

Int* begin_IntRange(IntRange* range);


//List
List* init_List(uint64_t size);

void set_List(List* this, Int* index, void* value);

void* get_List(List* this, Int* index);

List* add_List(List* this, void* el);

void uninit_List(List* arr);


