#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define CREATE_NUM_ARITH_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
    STRUCT_TYPE * (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs );

#define CREATE_NUM_ARITH_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)     \
    STRUCT_TYPE * (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs ) { \
        RAW_TYPE lhs_raw = lhs->raw_value;                          \
        RAW_TYPE rhs_raw = rhs->raw_value;                          \
        return init_ ## STRUCT_TYPE (lhs_raw OP rhs_raw);           \
    }

#define CREATE_NUM_CMP_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
    Bool * (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs );

#define CREATE_NUM_CMP_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)     \
    Bool * (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs ) { \
        RAW_TYPE lhs_raw = lhs->raw_value;                          \
        RAW_TYPE rhs_raw = rhs->raw_value;                          \
        return init_Bool(lhs_raw OP rhs_raw);           \
    }



//Int
typedef struct Int {
    int64_t raw_value;
} Int;

//Double
typedef struct Float {
    double raw_value;
} Float;


//Bool
typedef struct Bool {
    bool raw_value;
} Bool;

//String
typedef struct String {
    bool raw_is_on_heap;
    char* raw_value;
} String;

Int* init_Int(int64_t raw_value);

String* String_Int(Int* int_val);

void uninit_Int(Int* int_val);

CREATE_NUM_ARITH_FN_DECL(Int, int64_t, add, +)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, sub, -)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, mul, *)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, div, /)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, mod, %)

CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmplt, <)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmple, <=)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpne, !=)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpgt, >)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpge, >=)
CREATE_NUM_CMP_FN_DECL(Int, int64_t, cmpeq, ==)


Float* init_Float(double raw_value);

void uninit_Float(Float* int_val);

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


String* init_String(char* raw_value);

void uninit_String(String* int_val);

struct String* add_String(String* lhs, struct String* rhs);

struct Bool* init_Bool(bool raw_value);

void uninit_Bool(struct Bool* int_val);

struct Bool* add_Bool(Bool* lhs, Bool* rhs);

bool rawVal_Bool(Bool* this);





