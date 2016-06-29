#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "basic_funcs.h"
#include "gc_base.h"
#include "basic_types.h"

//TODO change how the String_* methods allocate memory

//Int
CREATE_PRIMITIVE_INIT_FN(Int, int64_t)

void uninit_Int(void* int_val)
{
    free(int_val);
}

void* String_Int(void* int_val)
{
    size_t buffer_size = 32;
    char* buffer       = (char*) malloc(buffer_size);
    //TODO error checking for sprintf, maybe
    snprintf(buffer, buffer_size, "%ld", ((Int*)int_val)->raw_value);

    String* result = init_String(buffer);
    result->raw_is_on_heap = true;

    return result;
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

void uninit_Float(void* float_val)
{
    free(float_val);
}

void* String_Float(void* float_val)
{
    size_t buffer_size = 32;
    char* buffer       = (char*) malloc(buffer_size);
    //TODO error checking for sprintf, maybe
    snprintf(buffer, buffer_size, "%f", ((Float*)float_val)->raw_value);
    String* result = init_String(buffer);
    result->raw_is_on_heap = true;

    return result;
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

void uninit_String(void* this)
{
    String* str_val = (String*)this;
    if( str_val->raw_is_on_heap )
    {
        free(str_val->raw_value);
    }
    free(str_val);
}

void* add_String(void* this,  va_list* rhs_obj)
{
    String* lhs   = (String*)this;
    String* rhs   = (String*)va_arg(*rhs_obj, void*);
    char* lhs_raw = lhs->raw_value;
    char* rhs_raw = rhs->raw_value;
   
    char * result_buf = malloc(strlen(lhs_raw) + strlen(rhs_raw) + 1);
    strcpy(result_buf, lhs_raw);
    strcat(result_buf, rhs_raw);

    String* result = init_String(result_buf);
    result->raw_is_on_heap = true;

    return result;
}


//Bool
CREATE_PRIMITIVE_INIT_FN(Bool, bool)

void* String_Bool(void* bool_val)
{
    size_t buffer_size = 5;
    char* buffer       = (char*) calloc(1, buffer_size);
    //TODO error checking for sprintf, maybe
    strcpy(buffer, (((Bool*)bool_val)->raw_value) ? "true" : "false");
    String* result = init_String(buffer);
    result->raw_is_on_heap = true;

    return result;
}

bool rawVal_Bool(void* this)
{
    return ((Bool*)this)->raw_value;
}

void uninit_Bool(void* bool_val)
{
    free(bool_val);
}


//IntRange
void* init_IntRange(void* start_obj, void* step_obj, void* end_obj)
{
    Int* start = start_obj;
    Int* step  = step_obj;
    Int* end   = end_obj;
    gc_base_t* base   = gc_malloc(sizeof(IntRange));
    IntRange* result  = (IntRange*) (base->raw_obj);
    result->back_ptr  = base;
    result->type_name = "IntRange";
    result->curr_val  = start;
    result->start     = start;
    result->step      = step;
    result->end       = end;

    assert( (((start->raw_value < end->raw_value) && (step->raw_value > 0)) 
          || ((start->raw_value > end->raw_value) && (step->raw_value < 0))
          || (start->raw_value == end->raw_value)) 
        && "Cannot get to end of iterator from start with current step value.");

    return result;
}

void* hasNext_IntRange(void* range_obj)
{
    IntRange* range = (IntRange*)range_obj;
    Bool* hasNext;
    if(range->step->raw_value > 0)
    {
        hasNext = init_Bool(range->curr_val->raw_value < range->end->raw_value);
    }
    else
    {
        hasNext = init_Bool(range->curr_val->raw_value > range->end->raw_value);
    }
    return hasNext;
}

void* next_IntRange(void* range_obj)
{
    IntRange* range = (IntRange*)range_obj;
    range->curr_val = init_Int(range->curr_val->raw_value + range->step->raw_value);
    return range->curr_val;
}

void* begin_IntRange(void* range_obj)
{
    IntRange* range = (IntRange*)range_obj;
    return init_Int(range->start->raw_value);
}


//List
void* init_List(uint64_t initial_size)
{
    gc_base_t* base   = gc_malloc(sizeof(List));
    List* result      = (List*) (base->raw_obj);
    result->back_ptr  = base;
    result->type_name = "List";
    result->size      = initial_size;
    result->capacity  = (initial_size + 1) * 2;
    result->raw_value = calloc(1, result->capacity * sizeof(void*));

    return result;
}

void set_List(void* this_obj, va_list* args_rest)
{
    List* this  = (List*)this_obj;
    Int* index  = (Int*)va_arg(*args_rest, void*);
    void* value = va_arg(*args_rest, void*);
    int64_t raw_ind = index->raw_value;

    assert(raw_ind < this->size && raw_ind >= 0 &&
            "List index out of bounds!");
    this->raw_value[raw_ind] = value;
}

void* get_List(void* this_obj, va_list* args_rest)
{
    List* this  = (List*)this_obj;
    Int* index  = (Int*)va_arg(*args_rest, void*);
    int64_t raw_ind = index->raw_value;

    assert(raw_ind < this->size && raw_ind >= 0 &&
            "List index out of bounds!");

    return this->raw_value[raw_ind];
}

void* add_List(void* this_obj, va_list* args_rest)
{
    List* this = (List*)this_obj;
    void* el   = va_arg(*args_rest, void*);
    if( (this->size + 1) == this->capacity ) 
    {
        this->capacity = this->capacity * 2;

        void** new_mem = calloc(1, this->capacity * sizeof(void*));

        if( !new_mem )
        {
            assert(false && "Memory allocation failed in add_List!");
        }

        memcpy(new_mem, this->raw_value, sizeof(void*) * this->size);
        free(this->raw_value);
        this->raw_value = new_mem;
    }
    this->raw_value[this->size] = el;
    this->size++;

    return this;
}

void uninit_List(void* arr)
{
    free(((List*)arr)->raw_value);
}




