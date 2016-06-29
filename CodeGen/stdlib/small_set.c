#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "fast_malloc.h"
#include "small_set.h"

small_set_t* small_set_init(void)
{
    small_set_t* result = (small_set_t*) fast_malloc(sizeof(small_set_t));
    return result;
}

void small_set_uninit(small_set_t* set)
{
    for( int i = 0; i < SET_ARR_SIZE; i++ )
    {
        set_list_node_t* curr_node = (set->set_arr[i]).next;
        set_list_node_t* next_node;

        while( curr_node != NULL )
        {
            next_node = curr_node->next;
            fast_free(curr_node);
            curr_node = next_node;
        }
    }
    fast_free(set);
}

static uint64_t small_set_hash(char* key)
{
    uint64_t result = 1;
    int i           = 0;

    while( key[i] != '\0' )
    {
        result += ((uint64_t)key[i]) * 31;
        i++;
    }

    return result % SET_ARR_SIZE;
}

/*small_set_list_get
 *
 * returns non-null if the key is in the list provided by list_head,
 * else returns null.
 * does not check list head.
 */
static set_list_node_t* small_set_list_get(set_list_node_t* list_head, 
                                            char* key)
{
    set_list_node_t* curr_node = list_head->next;

    while( curr_node != NULL )
    {
        if( strcmp(key, curr_node->key) == 0 )
        {
            return curr_node;
        }
        curr_node = curr_node->next;
    }

    return NULL;
}

void small_set_set(small_set_t* set, char* key, void* value)
{
    uint64_t index = small_set_hash(key);

    small_set_remove(set, key);

    set->size++;
    set_list_node_t* list_head = &(set->set_arr[index]); 
    set_list_node_t* new_node  = fast_malloc(sizeof(set_list_node_t)); 

    if( !new_node )
    {
        assert(false && "small_set element allocation failed!");
    }

    new_node->key   = key;
    new_node->value = value;

    new_node->next  = list_head->next;
    new_node->prev  = list_head;
    list_head->next  = new_node;

    if( new_node->next )
    {
        new_node->next->prev = new_node;
    }
}

void* small_set_get(small_set_t* set, char* key)
{
    uint64_t index        = small_set_hash(key);
    set_list_node_t* node = small_set_list_get(&(set->set_arr[index]), 
                                                key);
    return node ? node->value : NULL;
}

bool small_set_remove(small_set_t* set, char* key)
{
    set_list_node_t* list_node = small_set_get(set, key);

    if( list_node )
    {
        set->size--;

        if( list_node->next ) 
        {
            list_node->next->prev = list_node->prev;
        }
        //don't need to check for existence of prev since all lists have
        //head and it is assumed that head won't be passed in to remove
        list_node->prev->next = list_node->next;

        fast_free(list_node);

        return true;
    }

    return false;
}


