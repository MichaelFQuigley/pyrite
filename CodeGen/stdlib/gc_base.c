/* gc_base.c
 *
 * Michael Quigley
 * 
 */
#include <stdio.h>


#include "gc_base.h"
#include "fast_malloc.h"

static gc_list_t gc_list;

//object stack
//note: stack grows up
static gc_stack_t gc_stack;

//stack that holds count for number of alloced objects at each scope
static gc_scope_stack_t gc_scope_stack;

static inline gc_scope_stack_el_t* get_scope_stack_top()
{
    return &(gc_scope_stack.stack[gc_scope_stack.curr_index]);
}

//linked list helpers
static inline void add_node(gc_base_t* node)
{
    node->next         = gc_list.gc_list_head.next;
    node->prev         = &gc_list.gc_list_head;
    gc_list.gc_list_head.next = node;
    gc_list.size++;
}

static inline void remove_node(gc_base_t* node)
{
    GC_ASSERT(gc_list.size > 0 && "Trying to delete an object not in the list.");

    if( node->prev )
    {
        node->prev->next = node->next;
    }
    if( node->next )
    {
        node->next->prev = node->prev;
    }

    gc_list.size--;
}

//gc helpers
//
/* gc_free
 *
 * Frees object and any references to the object as well as removing it from the global linked
 * list of allocated objects.
 */
static inline void gc_free(void* val)
{
    remove_node((gc_base_t*) val);
    fast_free(((gc_base_t*)val)->raw_obj);
    fast_free(val);
}

static inline bool should_garbage_collect()
{
    return gc_list.size >  (MAX_STACK_SIZE / 2);
}

static inline void mark(gc_base_t* obj)
{
    if( obj )
    {
        if( !lang_core_get_obj_is_marked(obj) )
        {
            lang_core_set_obj_is_marked(obj, true);
           // printf("flags = %ld\n", obj->flags);
           // printf("ref   = %ld\n", obj->ref_size);
            for( int i = 0; i < obj->ref_size; i++ )
            {
                mark((gc_base_t*) obj->refs[i]);
            }
        }
    }
}

static inline void sweep()
{
    gc_base_t* curr_node = &(gc_list.gc_list_head); 
    gc_base_t* next_node = curr_node->next;
    while( next_node != NULL )
    {
        curr_node = next_node;
        next_node = curr_node->next;

        if( lang_core_get_obj_is_marked(curr_node) )
        {
            lang_core_set_obj_is_marked(curr_node, false);
        }
        else
        {
            gc_free(curr_node); 
        }
    }
}

void gc_mark_and_sweep(void)
{
    printf("collecting garbage\n");
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

    sweep();
}

void gc_init(void)
{
    gc_stack.curr_index       = 0;
    gc_scope_stack.curr_index = 0;
}

gc_base_t* gc_malloc(size_t size)
{
    void* raw_obj                 = fast_malloc(size);
    gc_base_t* base               = (gc_base_t*) fast_malloc(sizeof(gc_base_t));
   
    if( !base || !raw_obj )
    {
        return NULL;
    }

    base->raw_obj = raw_obj;
    lang_core_set_obj_is_marked(base, false);

    //add to stack
    GC_ASSERT(gc_stack.curr_index < MAX_STACK_SIZE &&
            gc_stack.curr_index >= 0 &&
            "Out of memory in virtual stack.");
    gc_stack.stack[gc_stack.curr_index] = base;
    gc_stack.curr_index++;

    //update scope stack
    gc_scope_stack.stack[gc_scope_stack.curr_index].num_anon_vars++;

    //add to linked list
    add_node(base);    

    return base;
}

void gc_push_scope(uint64_t num_named_vars_in_scope)
{
    GC_ASSERT(gc_scope_stack.curr_index < MAX_SCOPE_DEPTH - 1 &&
           gc_scope_stack.curr_index >= 0 &&
           "Scope too deep!");
   gc_scope_stack.curr_index++;

   gc_scope_stack_el_t* scope_el = get_scope_stack_top();
   scope_el->num_named_vars      = num_named_vars_in_scope;
   if( num_named_vars_in_scope > 0 )
   {
       scope_el->named_vars = fast_malloc(sizeof(gc_base_t) * num_named_vars_in_scope);
   }
   else
   {
       scope_el->named_vars = NULL;
   }
}

void gc_pop_scope(void)
{
    GC_ASSERT(gc_scope_stack.curr_index > 0 &&
            "Scope stack undeflow!");
    if( should_garbage_collect() )
    {
        gc_mark_and_sweep();
    }
    gc_scope_stack_el_t* scope_el = get_scope_stack_top();
    uint64_t num_alloced_in_scope = scope_el->num_anon_vars;

    gc_stack.curr_index -= num_alloced_in_scope;
    GC_ASSERT(gc_stack.curr_index >= 0 &&
            "Object stack undeflow!");

    scope_el->num_anon_vars  = 0;
    scope_el->num_named_vars = 0;
    fast_free(scope_el->named_vars);
    scope_el->named_vars = NULL;

    gc_scope_stack.curr_index--;
}

void gc_set_named_var_in_scope(gc_base_t* base, uint64_t index)
{
    gc_scope_stack_el_t* el = get_scope_stack_top();
    GC_ASSERT(index < el->num_named_vars && "Out of bounds var index!");
    el->named_vars[index] = base;
}
