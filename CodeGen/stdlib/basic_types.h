#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
//Arithmetic function macros
#define CREATE_NUM_ARITH_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
    void (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs, STRUCT_TYPE * result );

#define CREATE_NUM_ARITH_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)                           \
    void (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs, STRUCT_TYPE * result ) { \
        result->raw_value = lhs->raw_value OP rhs->raw_value; \
    }
//Compare function macros
#define CREATE_NUM_CMP_FN_DECL(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP) \
    void (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs, Bool * result );

#define CREATE_NUM_CMP_FN(STRUCT_TYPE, RAW_TYPE, FN_NAME, OP)                                      \
    void (FN_NAME ## _ ## STRUCT_TYPE) (STRUCT_TYPE * lhs, STRUCT_TYPE * rhs, Bool * result ) {    \
        result->raw_value = lhs->raw_value OP rhs->raw_value;                                      \
    }
//Initialization function macros
#define CREATE_PRIMITIVE_INIT_FN_DECL(STRUCT_TYPE, RAW_TYPE)    \
    void ( init_ ## STRUCT_TYPE) (RAW_TYPE raw_value, STRUCT_TYPE * val);

#define CREATE_PRIMITIVE_INIT_FN(STRUCT_TYPE, RAW_TYPE)                      \
    void ( init_ ## STRUCT_TYPE) (RAW_TYPE raw_value, STRUCT_TYPE * val) {  \
        val->raw_value = raw_value;                                          \
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


CREATE_PRIMITIVE_INIT_FN_DECL(Int, int64_t)

void String_Int(Int* int_val, String* result);

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

void String_Float(Float* float_val, String* result);

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

void add_String(String* lhs, String* rhs, String* result);

CREATE_PRIMITIVE_INIT_FN_DECL(Bool, bool)

void uninit_Bool(Bool* int_val);

void String_Bool(Bool* bool_val, String* result);

bool rawVal_Bool(Bool* this);





