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


//Int
typedef struct Int {
    int64_t raw_value;
} Int;

struct Int* init_Int(int64_t raw_value);

void uninit_Int(struct Int* int_val);

CREATE_NUM_ARITH_FN_DECL(Int, int64_t, add, +)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, sub, -)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, mul, *)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, div, /)
CREATE_NUM_ARITH_FN_DECL(Int, int64_t, mod, %)


//Double
typedef struct Float {
    double raw_value;
} Float;

struct Float* init_Float(double raw_value);

void uninit_Float(struct Float* int_val);

CREATE_NUM_ARITH_FN_DECL(Float, double, add, +)
CREATE_NUM_ARITH_FN_DECL(Float, double, sub, -)
CREATE_NUM_ARITH_FN_DECL(Float, double, mul, *)
CREATE_NUM_ARITH_FN_DECL(Float, double, div, /)


//String
typedef struct String {
    char* raw_value;
} String;

struct String* init_String(char* raw_value);

void uninit_String(struct String* int_val);

struct String* add_String(struct String* lhs, struct String* rhs);


//Bool
typedef struct Bool {
    bool raw_value;
} Bool;

struct Bool* init_Bool(bool raw_value);

void uninit_Bool(struct Bool* int_val);

struct Bool* add_Bool(struct Bool* lhs, struct Bool* rhs);

bool rawVal_Bool(struct Bool* this);





