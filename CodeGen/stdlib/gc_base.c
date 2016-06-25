/* gc_base.c
 *
 * Michael Quigley
 * 
 */
#include "gc_base.h"

static gc_list_t gc_list;

//object stack
//note: stack grows up
static gc_stack_t gc_stack;

//stack that holds count for number of alloced objects at each scope
static gc_scope_stack_t gc_scope_stack;

//linked list helpers
static void add_node(gc_base_t* node)
{
    node->next         = gc_list.gc_list_head.next;
    node->prev         = &gc_list.gc_list_head;
    gc_list.gc_list_head.next = node;
    gc_list.size++;
}

static void remove_node(gc_base_t* node)
{
    assert(gc_list.size > 0 && "Trying to delete an object not in the list.");

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
static bool should_garbage_collect()
{
    return gc_list.size > (MAX_STACK_SIZE / 2);
}

static void mark(gc_base_t* obj)
{
    if( !lang_core_get_obj_is_marked(obj) )
    {
        lang_core_set_obj_is_marked(obj, true);
        for( int i = 0; i < obj->ref_size; i++ )
        {
            mark((gc_base_t*) obj->refs[i]);
        }
    }
}

static void sweep()
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
    for(int i = 0; i < gc_stack.curr_index; i++)
    {
        mark(gc_stack.stack[i]);
    }

    sweep();
}

void gc_init(void)
{
    gc_stack.curr_index       = 0;
    gc_scope_stack.curr_index = 0;
}

void* gc_malloc(size_t size)
{
    static const int num_elements = 1;
    void* raw_obj                 = calloc(num_elements, size);
    gc_base_t* base               = (gc_base_t*) calloc(num_elements, sizeof(gc_base_t));
   
    if( !base || !raw_obj )
    {
        return NULL;
    }

    base->raw_obj = raw_obj;

    //add to stack
    assert(gc_stack.curr_index < MAX_STACK_SIZE &&
            gc_stack.curr_index >= 0 &&
            "Out of memory in virtual stack.");
    gc_stack.stack[gc_stack.curr_index] = base;
    gc_stack.curr_index++;

    //update scope stack
    gc_scope_stack.stack[gc_scope_stack.curr_index]++;

    //add to linked list
    add_node(base);    

    return base;
}

static void gc_free(void* val)
{
    remove_node((gc_base_t*) val);
    free(((gc_base_t*)val)->raw_obj);
    free(val);
}


void gc_push_scope(void)
{
    assert(gc_scope_stack.curr_index < MAX_SCOPE_DEPTH - 1 &&
           gc_scope_stack.curr_index >= 0 &&
           "Scope too deep!");
   gc_scope_stack.curr_index++; 
}

void gc_pop_scope(void)
{
    assert(gc_scope_stack.curr_index > 0 &&
            "Scope stack undeflow!");

    uint64_t num_alloced_in_scope = gc_scope_stack.stack[gc_scope_stack.curr_index];

    gc_stack.curr_index -= num_alloced_in_scope;
    assert(gc_stack.curr_index >= 0 &&
            "Object stack undeflow!");


    gc_scope_stack.stack[gc_scope_stack.curr_index] = 0;
    gc_scope_stack.curr_index--;
}


