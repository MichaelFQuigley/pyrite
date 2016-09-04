#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "basic_funcs.h"
#include "gc_base.h"
#include "fast_malloc.h"
#include "basic_types.h"

#define NUM_PREALLOC_INTS  256
#define NUM_PREALLOC_BOOLS 2

static void* prealloc_ints[NUM_PREALLOC_INTS];
static void* prealloc_bools[NUM_PREALLOC_BOOLS];

int initialize_types(void)
{
    for( int64_t i = 0; i < NUM_PREALLOC_INTS; i++ )
    {
        Int* prealloced_int       = gc_malloc(sizeof(Int)); 
        prealloced_int->uninit    = NULL;
        prealloced_int->get_refs  = NULL;
        prealloced_int->raw_value = i;
        prealloc_ints[i] = prealloced_int;
    }  

    for( int64_t i = 0; i < NUM_PREALLOC_BOOLS; i++ )
    {
        Bool* prealloced_bool      = gc_malloc(sizeof(Bool)); 
        prealloced_bool->uninit    = NULL;
        prealloced_bool->get_refs  = NULL;
        prealloced_bool->raw_value = (bool)i;
        prealloc_bools[i] = prealloced_bool;
    }  

    return 0;
}

int uninitialize_types(void)
{
    return 0;
}


//Int
void * init_Int(int64_t raw_value)
{
    if( raw_value >= NUM_PREALLOC_INTS )
    {
        CREATE_PRIMITIVE_INIT_BLOCK(Int, int64_t, obj);
        return obj;
    }
    else
    {
        return prealloc_ints[raw_value];
    }
}

void uninit_Int(void* int_val)
{
    return;
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
void * init_Float(double raw_value)
{
    CREATE_PRIMITIVE_INIT_BLOCK(Float, double, obj);

    return obj;
}


void uninit_Float(void* float_val)
{
    return;
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
void * init_String(char* raw_value)
{
    CREATE_PRIMITIVE_INIT_BLOCK(String, char*, obj);
    obj->uninit = uninit_String;
    obj->raw_is_on_heap = false;
    return obj;
}



void uninit_String(void* this)
{
    String* str_val = (String*)this;
    if( str_val->raw_is_on_heap )
    {
        free(str_val->raw_value);
    }
}

void* add_String(void* this,  void* rhs_obj)
{
    String* lhs   = (String*)this;
    String* rhs   = (String*)rhs_obj;
    char* lhs_raw = lhs->raw_value;
    char* rhs_raw = rhs->raw_value;
   
    char * result_buf = malloc(strlen(lhs_raw) + strlen(rhs_raw) + 1);
    strcpy(result_buf, lhs_raw);
    strcat(result_buf, rhs_raw);

    String* result = init_String(result_buf);
    result->raw_is_on_heap = true;

    return result;
}

void* String_String(void* this)
{
    return this;
}
//Bool
void * init_Bool(bool raw_value)
{
    return prealloc_bools[raw_value ? 1 : 0];
}

CREATE_NUM_ARITH_FN(Bool, bool, and, &)
CREATE_NUM_ARITH_FN(Bool, bool, or, |)
CREATE_NUM_ARITH_FN(Bool, bool, xor, ^)



void* String_Bool(void* bool_val)
{
    size_t buffer_size = 5;
    char* buffer       = (char*) malloc(buffer_size);
    buffer[buffer_size - 1] = '\0';
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
    return;
}


//IntRange
void* init_IntRange(void* start_obj, void* step_obj, void* end_obj)
{
    Int* start = start_obj;
    Int* step  = step_obj;
    Int* end   = end_obj;
    IntRange* result  = gc_malloc(sizeof(IntRange));
    result->uninit    = NULL;
    result->get_refs  = NULL;
    result->curr_val  = start;
    result->start     = start;
    result->step      = step;
    result->end       = end;
/*
    assert( (((start->raw_value < end->raw_value) && (step->raw_value > 0)) 
          || ((start->raw_value > end->raw_value) && (step->raw_value < 0))
          || (start->raw_value == end->raw_value)) 
        && "Cannot get to end of iterator from start with current step value.");*/

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
    return range->start; 
}

void uninit_IntRange(void* this)
{
    return;
}


//List
void* init_List(uint64_t initial_size)
{
    List* result      = (List*) gc_malloc(sizeof(List));
    result->uninit    = NULL;
    result->get_refs  = get_refs_List;
    result->size      = initial_size;
    result->capacity  = initial_size; //(initial_size + 1) * 2;
    result->raw_value = fast_zalloc(result->capacity * sizeof(void*));
    result->next_itt_index = 0;

    return result;
}

void set_List(void* this_obj, void* index_obj, void* value)
{
    List* this  = (List*)this_obj;
    Int* index  = (Int*)index_obj;
    int64_t raw_ind = index->raw_value;

    assert(raw_ind < this->size && raw_ind >= 0 &&
            "List index out of bounds!");
    this->raw_value[raw_ind] = value;
}

void* get_List(void* this_obj, void* index_obj)
{
    List* this  = (List*)this_obj;
    Int* index  = (Int*)index_obj;
    int64_t raw_ind = index->raw_value;

    assert(raw_ind < this->size && raw_ind >= 0 &&
            "List index out of bounds!");
    return this->raw_value[raw_ind];
}

void* add_List(void* this_obj, void* el)
{
    List* this = (List*)this_obj;
    List* new_list = init_List(this->size + 1);

    for(uint64_t i = 0; i < this->size; i++)
    {
        new_list->raw_value[i] = this->raw_value[i];
    }

    new_list->raw_value[this->size] = el;

    return new_list;
}

void* String_List(void* this)
{
    return init_String("String_List not implemented!");
}

void uninit_List(void* this)
{
    //free(((List*)this)->raw_value);
    fast_free(((List*)this)->raw_value);
}

void** get_refs_List(void* this)
{
    List* list  = (List*)this;
    void** refs = (void**)malloc(sizeof(List) * (list->size + 1));

    assert(refs && "Allocation failed in get_refs_List");

    for(uint32_t i = 0; i < list->size; i++)
    {
        refs[i] = list->raw_value[i];
    }
    refs[list->size] = NULL;

    return refs;
}

void* hasNext_List(void* this)
{
    return init_Bool(((List*)this)->next_itt_index < ((List*)this)->size);
}

void* next_List(void* this)
{
    ((List*)this)->next_itt_index++;
    return ((List*)this)->raw_value[((List*)this)->next_itt_index];
}

void* begin_List(void* this)
{
    ((List*)this)->next_itt_index = 0;
    if(((List*)this)->size > 0)
    {
        return ((List*)this)->raw_value[0];
    }
    else
    {
        return NULL;
    }
}
