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

#define MAX_STACK_SIZE (1 << 22) //4M objects
#define MAX_SCOPE_DEPTH (1 << 10) //1024

#define GC_DEBUG

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


//XXX gc_base should be word aligned
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
    struct gc_base* prev;
    struct gc_base* next;
} gc_base_t;

typedef struct 
{
    //global gc linked list
    //format of list:
    //  &head -> first node -> second node -> ... -> &tail
    gc_base_t gc_list_head;
    gc_base_t gc_list_tail;
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
    //func_scope_offset specifies this stack element's
    //distance below the nearest function scope
    uint64_t func_scope_offset;
    //array of named variables
    gc_base_t** named_vars;
} gc_scope_stack_el_t;

typedef struct
{
    gc_scope_stack_el_t stack[MAX_SCOPE_DEPTH];
    int64_t curr_index;
} gc_scope_stack_t;

static inline uint64_t lang_core_get_obj_is_marked(gc_base_t* obj)
{
    return (obj->flags & is_marked_mask);
}

static inline void lang_core_set_obj_is_marked(gc_base_t* this, uint64_t is_marked)
{
    this->flags &= ~(1);
    this->flags |= (is_marked & 1); 
}

/* gc_malloc
 *
 * Allocates sizeof(gc_base_t) gc wrapper around object and size bytes for object
 * Adds new object into global linked list, and pushes pointer on stack.
 * Sets is_marked bit in flags field to 'false'.
 *
 * On failure, null is returned and nothing is added to the linked lists or stacks.
 *
 * Returns gc_base_t with raw_obj pointing to alloced memory
 */
void* gc_malloc(size_t size);

//must be called externally before any other function
int gc_init(void);

//should be called every time a new scope is introduced
void gc_push_func_scope(uint64_t num_named_vars);
void gc_push_loop_scope(void);

//should be called every time a scope is removed
void gc_pop_scope(void);

//sets value of var in scope
void gc_set_named_var_in_scope(void* named_var, uint64_t index);

//Checks the newest generation to see if a garbage collection job should happen.
//If so, then a job is initiated.
void gc_check(void);
#endif
