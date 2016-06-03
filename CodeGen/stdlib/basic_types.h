#include <stdint.h>
#include <stdlib.h>

//Int
typedef struct Int {
    int64_t raw_value;
} Int;

struct Int* init_Int(int64_t raw_value);

void uninit_Int(struct Int* int_val);

struct Int* add_Int(struct Int* lhs, struct Int* rhs);

//Double
typedef struct Float {
    double raw_value;
} Float;

struct Float* init_Float(double raw_value);

void uninit_Float(struct Float* int_val);

struct Float* add_Float(struct Float* lhs, struct Float* rhs);

//String
typedef struct String {
    char* raw_value;
} String;

struct String* init_String(char* raw_value);

void uninit_String(struct String* int_val);

struct String* add_String(struct String* lhs, struct String* rhs);




