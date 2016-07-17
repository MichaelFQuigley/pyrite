/* gc_base.c
 *
 * Michael Quigley
 * 
 */
#include <stdio.h>

#include "basic_types.h"
#include "basic_funcs.h"
#include "gc_base.h"
#include "fast_malloc.h"
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#define NUM_GENERATIONS 3
static gc_list_t gc_list[NUM_GENERATIONS];

//object stack
//note: stack grows up
static gc_stack_t gc_stack;

//stack that holds count for number of alloced objects at each scope
static gc_scope_stack_t gc_scope_stack;

static inline gc_scope_stack_el_t* get_scope_stack_top()
{
    return &(gc_scope_stack.stack[gc_scope_stack.curr_index]);
}

static inline gc_scope_stack_el_t* get_nearest_func_scope()
{
    gc_scope_stack_el_t* top = get_scope_stack_top();
    return &(gc_scope_stack.stack[gc_scope_stack.curr_index - top->func_scope_offset]);
}


//linked list helpers
static inline void add_node(gc_base_t* node, int generation)
{
    node->next         = gc_list[generation].gc_list_head.next;
    node->prev         = &(gc_list[generation].gc_list_head);
    gc_list[generation].gc_list_head.next = node;
    gc_list[generation].size++;
}

static inline void remove_node(gc_base_t* node, int generation)
{
    GC_ASSERT(gc_list[generation].size > 0 
            && "Trying to delete an object not in the list.");

    node->prev->next = node->next;

    if( node->next )
    {
        node->next->prev = node->prev;
    }

    gc_list[generation].size--;
}

static inline void* gc_get_raw_obj(gc_base_t* base)
{
    return ((void*)base) + sizeof(gc_base_t);
}

static inline gc_base_t* get_gc_base_ptr(void* val)
{
    return val - sizeof(gc_base_t);
}

//gc helpers
//
/* gc_free
 *
 * Frees object and any references to the object as well as removing it from the global linked
 * list of allocated objects.
 */
static inline void gc_free(gc_base_t* val, int generation)
{
    remove_node(val, generation);
    Base* raw_obj = (Base*)gc_get_raw_obj(val);

    if( raw_obj->uninit )
    {
        raw_obj->uninit(raw_obj);
    }

    fast_free(val);
}

static inline bool should_garbage_collect(int generation)
{
    return gc_list[generation].size >  ((2 * MAX_STACK_SIZE) / 3);
}

static void mark(gc_base_t* obj)
{
    if( obj )
    {
        if( !lang_core_get_obj_is_marked(obj) )
        {
            lang_core_set_obj_is_marked(obj, 1);
            Base* base_obj = gc_get_raw_obj(obj);

            if( base_obj->get_refs )
            {
                void** obj_refs         = base_obj->get_refs(base_obj);
                uint32_t curr_ref_index = 0;
                while( obj_refs[curr_ref_index] != NULL )
                {
                    mark((gc_base_t*) get_gc_base_ptr(obj_refs[curr_ref_index]));
                    curr_ref_index++;
                }
                free(obj_refs);
            }
        }
    }
}

static inline void sweep(int generation)
{
    gc_base_t* curr_node = &(gc_list[generation].gc_list_head); 
    gc_base_t* next_node = curr_node->next;
    while( next_node != &(gc_list[generation].gc_list_tail) )
    {
        curr_node = next_node;
        next_node = curr_node->next;

        if( lang_core_get_obj_is_marked(curr_node) )
        {
            lang_core_set_obj_is_marked(curr_node, 0);
        }
        else
        {
            gc_free(curr_node, generation); 
        }
    }

    //move survivors to older generation if possible
    if( generation + 1 < NUM_GENERATIONS )
    {
        gc_base_t* old_gen_head = &(gc_list[generation].gc_list_head); 
        gc_base_t* old_gen_tail = &(gc_list[generation].gc_list_tail); 
        gc_base_t* new_gen_head = &(gc_list[generation + 1].gc_list_head); 

        if( gc_list[generation].size > 0 )
        {
            old_gen_tail->prev->next = new_gen_head->next;
            new_gen_head->next       = old_gen_head->next;
            new_gen_head->next->prev = new_gen_head;

            gc_list[generation + 1].size += gc_list[generation].size;
            gc_list[generation].size = 0;
            old_gen_tail->prev = old_gen_head;
            old_gen_head->next = old_gen_tail;
        }
    }
}

static void gc_mark_and_sweep(void)
{
    static uint64_t gc_count = 0;
    //mark all anon vars
    for(int i = 0; i < gc_stack.curr_index; i++)
    {
        mark(gc_stack.stack[i]);
    }
    //mark all named vars
    for(int i = 0; i <= gc_scope_stack.curr_index; i++)
    {
        gc_scope_stack_el_t* scope_el = &(gc_scope_stack.stack[i]);
        for(int j = 0; j < scope_el->num_named_vars; j++)
        {
            mark(scope_el->named_vars[j]);
        }
    }

    sweep(0);
    for(int i = 1; i < NUM_GENERATIONS; i++)
    {
        if( (gc_count % (i*2) == 0) && should_garbage_collect(i) )
        {
            sweep(i);
        }
    }

    gc_count++;
}

int gc_init(void)
{
    gc_stack.curr_index       = 0;
    gc_scope_stack.curr_index = 0;

    for(int i = 0; i < NUM_GENERATIONS; i++)
    {
        gc_list[i].gc_list_head.next = &(gc_list[i].gc_list_tail);
        gc_list[i].gc_list_tail.prev = &(gc_list[i].gc_list_head);
    }

    return 0;
}

void* gc_malloc(size_t size)
{
    gc_base_t* base = (gc_base_t*) fast_malloc(sizeof(gc_base_t) + size);
    void* raw_obj   = ((void*)base) + sizeof(gc_base_t);
    if( !base )
    {
        return NULL;
    }

    lang_core_set_obj_is_marked(base, 0);

    //add to stack
    GC_ASSERT(gc_stack.curr_index < MAX_STACK_SIZE &&
            gc_stack.curr_index >= 0 &&
            "Out of memory in virtual stack.");
    gc_stack.stack[gc_stack.curr_index] = base;
    gc_stack.curr_index++;

    //update scope stack
    gc_scope_stack.stack[gc_scope_stack.curr_index].num_anon_vars++;

    //add to linked list
    add_node(base, 0);    

    return raw_obj;
}

void gc_push_func_scope(uint64_t num_named_vars_in_scope)
{
    GC_ASSERT(gc_scope_stack.curr_index < MAX_SCOPE_DEPTH - 1 &&
           gc_scope_stack.curr_index >= 0 &&
           "Scope too deep!");
   gc_scope_stack.curr_index++;

   gc_scope_stack_el_t* scope_el = get_scope_stack_top();
   scope_el->num_named_vars      = num_named_vars_in_scope;
   scope_el->func_scope_offset   = 0;
   if( num_named_vars_in_scope > 0 )
   {
       scope_el->named_vars = fast_zalloc(sizeof(gc_base_t) * num_named_vars_in_scope);
   }
   else
   {
       scope_el->named_vars = NULL;
   }
}

void gc_push_loop_scope(void)
{
    GC_ASSERT(gc_scope_stack.curr_index < MAX_SCOPE_DEPTH - 1 &&
           gc_scope_stack.curr_index >= 0 &&
           "Scope too deep!");
   gc_scope_stack_el_t* last_scope = get_scope_stack_top();
   gc_scope_stack.curr_index++;

   gc_scope_stack_el_t* scope_el = get_scope_stack_top();
   scope_el->num_named_vars      = 0;
   scope_el->func_scope_offset   = last_scope->func_scope_offset + 1;
}


void gc_pop_scope(void)
{
    GC_ASSERT(gc_scope_stack.curr_index > 0 &&
            "Scope stack undeflow!");

    if( should_garbage_collect(0) )
    {
        gc_mark_and_sweep();
    }

    gc_scope_stack_el_t* scope_el = get_scope_stack_top();
    uint64_t num_alloced_in_scope = scope_el->num_anon_vars;

    gc_stack.curr_index -= num_alloced_in_scope;
    GC_ASSERT(gc_stack.curr_index >= 0 &&
            "Object stack undeflow!");

    scope_el->num_anon_vars  = 0;

    if( scope_el->num_named_vars > 0)
    {
        scope_el->num_named_vars = 0;
        fast_free(scope_el->named_vars);
    }
    scope_el->named_vars = NULL;

    gc_scope_stack.curr_index--;
}

void gc_set_named_var_in_scope(void* named_var, uint64_t index)
{
    gc_scope_stack_el_t* func_scope = get_nearest_func_scope();
    GC_ASSERT(index < func_scope->num_named_vars && "Out of bounds var index!");
    func_scope->named_vars[index] = get_gc_base_ptr(named_var);
}


