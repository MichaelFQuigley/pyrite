#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "gc_base.h"

// Arithmetic function macros
#define CREATE_NUM_ARITH_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
  void*(FN_NAME##_##STRUCT_TYPE)(void* lhs_obj, void* rhs_obj);

#define CREATE_NUM_ARITH_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)  \
  void*(FN_NAME##_##STRUCT_TYPE)(void* lhs_obj, void* rhs_obj) { \
    STRUCT_TYPE* lhs = (STRUCT_TYPE*)lhs_obj;                    \
    STRUCT_TYPE* rhs = (STRUCT_TYPE*)rhs_obj;                    \
    return init_##STRUCT_TYPE(lhs->raw_value OP rhs->raw_value); \
  }
// Compare function macros
#define CREATE_NUM_CMP_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
  void*(FN_NAME##_##STRUCT_TYPE)(void* lhs_obj, void* rhs_obj);

#define CREATE_NUM_CMP_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)    \
  void*(FN_NAME##_##STRUCT_TYPE)(void* lhs_obj, void* rhs_obj) { \
    STRUCT_TYPE* lhs = (STRUCT_TYPE*)lhs_obj;                    \
    STRUCT_TYPE* rhs = (STRUCT_TYPE*)rhs_obj;                    \
    return init_Bool(lhs->raw_value OP rhs->raw_value);          \
  }
// Initialization function macros
#define CREATE_PRIMITIVE_INIT_FN_DECL(STRUCT_TYPE, RAW_TYPE) \
  void*(init_##STRUCT_TYPE)(RAW_TYPE raw_value);

#define CREATE_PRIMITIVE_INIT_BLOCK(STRUCT_TYPE, RAW_TYPE, OBJ_OUT) \
  STRUCT_TYPE* OBJ_OUT;                                             \
  do {                                                              \
    (OBJ_OUT) = (STRUCT_TYPE*)gc_malloc(sizeof(STRUCT_TYPE));       \
    (OBJ_OUT)->raw_value = raw_value;                               \
    (OBJ_OUT)->uninit = NULL;                                       \
    (OBJ_OUT)->get_refs = NULL;                                     \
  } while (0);

#define BASE_VALS                                                       \
  void (*uninit)(void*);                                                \
  /*get_refs returns an array of references that the object contains.   \
   * The end of the array is determined by the                          \
   * last element being a NULL value. It is the caller's responsibility \
   * to free the memory that is allocated by get_refs.*/                \
  void** (*get_refs)(void*);

typedef struct Base { BASE_VALS } Base;

// Int
typedef struct Int {
  BASE_VALS

  int64_t raw_value;
} Int;

// Double
typedef struct Float {
  BASE_VALS

  double raw_value;
} Float;

// Bool
typedef struct Bool {
  BASE_VALS

  bool raw_value;
} Bool;

// String
typedef struct String {
  BASE_VALS

  bool raw_is_on_heap;
  char* raw_value;
} String;

// IntRange
typedef struct IntRange {
  BASE_VALS

  Int* curr_val;
  Int* start;
  Int* step;
  Int* end;
} IntRange;

typedef struct List {
  BASE_VALS
  // size is actual size of array
  uint64_t size;
  // capacity represents number of elements are supported by currently allocated
  // array
  size_t capacity;
  uint64_t next_itt_index;
  void** raw_value;
} List;

// initialize_types should be called before any other
// function from this file
int initialize_types(void);

int uninitialize_types(void);

CREATE_PRIMITIVE_INIT_FN_DECL(Int, int64_t)

void* String_Int(void* int_val);

void uninit_Int(void* int_val);

/* neg_Int: unary negation function. */
void* neg_Int(void* this);

CREATE_NUM_ARITH_FN_DECL(Int, int64_t, add, +)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, sub, -)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, mul, *)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, div, / )
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, mod, % )
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, and, &)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, or, | )
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, xor, ^)

CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmplt, < )
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmple, <= )
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpne, != )
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpgt, > )
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpge, >= )
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpeq, == )

CREATE_PRIMITIVE_INIT_FN_DECL(Float, double)

void uninit_Float(void* int_val);

void* String_Float(void* float_val);

void* neg_Float(void* this);

CREATE_NUM_ARITH_FN_DECL(Float, double, add, +)
CREATE_NUM_ARITH_FN_DECL(Float, double, sub, -)
CREATE_NUM_ARITH_FN_DECL(Float, double, mul, *)
CREATE_NUM_ARITH_FN_DECL(Float, double, div, / )

CREATE_NUM_CMP_FN_DECL(Float, double, cmplt, < )
CREATE_NUM_CMP_FN_DECL(Float, double, cmple, <= )
CREATE_NUM_CMP_FN_DECL(Float, double, cmpne, != )
CREATE_NUM_CMP_FN_DECL(Float, double, cmpgt, > )
CREATE_NUM_CMP_FN_DECL(Float, double, cmpge, >= )
CREATE_NUM_CMP_FN_DECL(Float, double, cmpeq, == )

CREATE_PRIMITIVE_INIT_FN_DECL(String, char*)

void uninit_String(void* int_val);

void* add_String(void* this, void* rhs_obj);

void* String_String(void* this);

CREATE_PRIMITIVE_INIT_FN_DECL(Bool, bool)

CREATE_NUM_ARITH_FN_DECL(Bool, bool, and, &)
CREATE_NUM_ARITH_FN_DECL(Bool, bool, or, | )
CREATE_NUM_ARITH_FN_DECL(Bool, bool, xor, ^)

void uninit_Bool(void* bool_val);

void* String_Bool(void* bool_val);

bool rawVal_Bool(void* this);

// IntRange
void* init_IntRange(void* start_obj, void* step_obj, void* end_obj);

void uninit_IntRange(void* this);

void* hasNext_IntRange(void* range_obj);

void* next_IntRange(void* range_obj);

void* begin_IntRange(void* range_obj);

// List
void* init_List(uint64_t initial_size);

void uninit_List(void* this);

void set_List(void* this_obj, void* index_obj, void* value);

void* get_List(void* this_obj, void* index_obj);

void* add_List(void* this_obj, void* other_list);

void* String_List(void* this);

void uninit_List(void* arr);

void** get_refs_List(void* this);

void* hasNext_List(void* this);

void* next_List(void* this);

void* begin_List(void* this);

#endif
