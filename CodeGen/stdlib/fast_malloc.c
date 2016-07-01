#include <stdint.h>
#include <string.h>

#include "fast_malloc.h"

#define MEM_ALIGN 16
#define MAX_MEM_SIZE (64 * MEM_ALIGN)

static fast_mem_node_t mem_slabs[(MAX_MEM_SIZE / MEM_ALIGN) + 1];

static inline void push_slab_head(void* mem)
{
    fast_mem_node_t* mem_node = (fast_mem_node_t*)(mem - sizeof(fast_mem_node_t));
    size_t size               = mem_node->size;

    if( size > MAX_MEM_SIZE )
    {
        free(mem_node);
        return;
    }

    fast_mem_node_t* old_head        = mem_slabs[size / MEM_ALIGN].next;
    mem_slabs[size / MEM_ALIGN].next = mem_node;
    mem_node->next                   = old_head;
}

static inline void* pop_slab_head(size_t size)
{
    void* result = NULL;

    if( size > MAX_MEM_SIZE )
    {
        return NULL;
    }

    fast_mem_node_t* mem_node =  mem_slabs[size / MEM_ALIGN].next;

    if( !mem_node )
    {
        return NULL;
    }
    else
    {
        mem_slabs[size / MEM_ALIGN].next = mem_node->next;
    }

    mem_node->next = NULL;
    mem_node->size = size;
    result = mem_node;
    result += sizeof(fast_mem_node_t);

    return result;
}

void* fast_malloc(size_t size)
{
    void* result_mem = NULL;

    //force memory to be aligned
    if( size % MEM_ALIGN != 0 )
    {
        size += (MEM_ALIGN - (size % MEM_ALIGN));
    }

    void* new_mem = pop_slab_head(size);
    //if size > MAX_MEM_SIZE, memory chunk can't be put into pool
    //so there is no point in looking in pool
    if( !new_mem )
    {
        fast_mem_node_t* mem_node = malloc(size + sizeof(fast_mem_node_t));
        mem_node->size = size;
        result_mem = mem_node;
        result_mem += sizeof(fast_mem_node_t);
    }
    else
    {
        result_mem = new_mem;
    }

    return result_mem; 
}

void* fast_zalloc(size_t size)
{
    void* result_mem = fast_malloc(size); 

    if( result_mem )
    {
        memset(result_mem, 0, size);
    }

    return result_mem;
}

void fast_free(void* mem)
{
    push_slab_head(mem);
}
