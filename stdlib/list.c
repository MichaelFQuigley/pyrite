
#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"

extern struct list;

list_t* list_init()
{
    list_t* list_inst = calloc(1, sizeof(list_t));
    assert(list_inst != 0 && "List allocation failed");

    (list_inst->head).next = &(list_inst->tail);
    (list_inst->tail).prev = &(list_inst->head);

    return list_inst;
}

void list_uninit(list_t* list_inst)
{
    //TODO: free elements of list
    free(list_inst);
}

void list_add(list_t* list_inst, 
                union list_value element, 
                list_element_types_t element_type)
{
    list_node_t* list_node = calloc(1, sizeof(list_node_t));
    assert(list_node != 0 && "List addition failed");

    list_node->element.type  = element_type;
    list_node->element.value = element;
    list_node->prev          = (list_inst->tail).prev;
    list_node->next          = &(list_inst->tail);
    (list_inst->tail).prev   = list_node;
    list_node->prev->next    = list_node;

    list_inst->size++;
}

void print_list(list_t* list_inst)
{
    list_node_t *curr_node;
    curr_node = list_inst->head.next;

    for(int i = 0; i < list_inst->size; i++)
    {
        switch(curr_node->element.type)
        {
            case INT64_TYPE:
                printf("list el: %ld\n", curr_node->element.value.i);
                break;
            case DOUBLE_TYPE:
                printf("list el: %f\n", curr_node->element.value.d);
                break;
            case PTR_TYPE:
                break;
            default:
                assert(1 && "Print not implemented for type");
                break;
        }
        curr_node = curr_node->next;
    }
}

/*
int main()
{
    list_t *list_inst = list_init();
    list_add(list_inst, (union list_value)1.1, DOUBLE_TYPE);
    list_add(list_inst, (union list_value)100.0, DOUBLE_TYPE);
    list_add(list_inst, (union list_value)((int64_t)10), INT64_TYPE);
    printf("starting\n");
    print_list(list_inst);
    return 0;
}*/
