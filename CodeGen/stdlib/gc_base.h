/* gc_base.h
 *
 * Michael Quigley
 *
 * Defines gc_base_t struct type to be used in garbage collection as well as
 * malloc and free functions that support garbage collection.
 * 
 */

#ifndef GC_BASE_H
#define GC_BASE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define MAX_STACK_SIZE (1 << 24) //32 M objects
#define MAX_SCOPE_DEPTH (1 << 10) //1024

//#define GC_DEBUG

#ifdef GC_DEBUG
    #define GC_ASSERT(COND) \
        assert(COND);
#else
    #define GC_ASSERT(COND) {do {} while(0);}
#endif

typedef const uint64_t bits_t;

static bits_t flags_num_bits_obj       = (sizeof(uint64_t)*8);
static bits_t flags_num_bits_is_marked = 1;
static bits_t flags_num_bits_size      = flags_num_bits_obj - flags_num_bits_is_marked;
static bits_t is_marked_mask           = 0x1;
static bits_t size_mask                = ~(is_marked_mask);



typedef struct gc_base
{
    /*
     * flags layout
     * |----------------------------|---------------------------|
     * | 63 62 ... 1 = obj size bits| 0 = is marked bit (for GC)| 
     * |____________________________|___________________________|
     * 
     */
    uint64_t flags;
    uint64_t ref_size;
    //refs: array of other objects this object references
    void**    refs;
    struct gc_base* prev;
    struct gc_base* next;

    //raw object pointer
    void* raw_obj;
} gc_base_t;

typedef struct 
{
    //global gc linked list
    //format of list:
    //  head -> first node -> second node -> ...
    gc_base_t gc_list_head;
    uint64_t size;
} gc_list_t;

typedef struct
{
    gc_base_t* stack[MAX_STACK_SIZE];
    int64_t curr_index;
} gc_stack_t;

typedef struct
{
    uint64_t num_anon_vars;
    uint64_t num_named_vars;
    //array of named variables
    gc_base_t** named_vars;
} gc_scope_stack_el_t;

typedef struct
{
    gc_scope_stack_el_t stack[MAX_SCOPE_DEPTH];
    int64_t curr_index;
} gc_scope_stack_t;


static inline uint64_t lang_core_get_obj_size(uint64_t flags)
{
    return ((flags & flags_num_bits_size) >> flags_num_bits_is_marked);
}

static inline uint64_t lang_core_get_obj_is_marked(gc_base_t* obj)
{
    return (obj->flags & flags_num_bits_is_marked);
}

static inline void lang_core_set_obj_size(gc_base_t* this, uint64_t size)
{
    this->flags &= ~(size_mask);
    this->flags |= (size << flags_num_bits_size);
}

static inline void lang_core_set_obj_is_marked(gc_base_t* this, bool is_marked)
{
    if( is_marked )
    {
        this->flags |= 1; 
    }
    else
    {
        this->flags &= ~(1);
    }
}

/* gc_malloc
 *
 * Allocates sizeof(gc_base_t) gc wrapper around object and size bytes for object
 * Adds new object into global linked list, and pushes pointer on stack.
 * Sets is_marked bit in flags field to 'false'.
 * The refs and ref_size fields are zeroed out since it is the responsibility
 * of the initialization function to set these.
 *
 * On failure, null is returned and nothing is added to the linked lists or stacks.
 *
 * Returns gc_base_t with raw_obj pointing to zero alloced memory
 */
gc_base_t* gc_malloc(size_t size);

//must be called externally before any other function
void gc_init(void);

//runs mark and sweep gc algorithm
void gc_mark_and_sweep(void);

//should be called every time a new scope is introduced
void gc_push_scope(uint64_t num_named_vars);

//should be called every time a scope is removed
void gc_pop_scope(void);

//sets value of var in scope
void gc_set_named_var_in_scope(gc_base_t* base, uint64_t index);

#endif
