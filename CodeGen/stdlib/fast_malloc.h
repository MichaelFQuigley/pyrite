/* fast_malloc.h
 *
 * Michael Quigley
 *
 * Fast memory allocator for small chunks fof memory. Uses pools of previously freed memory.
 * Memory that is malloced is also memset to zeros.
 */

#ifndef FAST_MALLOC_H
#define FAST_MALLOC_H

#include <stdlib.h>

/* fast_mem_node_t:
 *
 * struct that sits at beginning of alloced memory to keep track of it in linked list
 *
 */
typedef struct fast_mem_node 
{
    uint64_t size;
    struct fast_mem_node* prev;
    struct fast_mem_node* next;
} fast_mem_node_t;

/*fast_malloc:
 *
 * returns pointer to zero alloced memory on success
 *
 * returns NULL on failure
*/
void* fast_malloc(size_t size);

/* fast_free:
 *
 * frees memory alloced by fast malloc and adds it to memory pool if possible
 */
void fast_free(void* mem);

#endif
