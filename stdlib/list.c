
#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"

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

static void list_add(list_t* list_inst, 
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

void list_add_int(list_t* list_inst, int64_t element)
//void list_add_int(list_t* list_inst, union list_value element)
{
    list_add(list_inst, (union list_value) element, INT64_TYPE);
}


void list_add_float(list_t* list_inst, double element)
//void list_add_float(list_t* list_inst, union list_value element)
{
    list_add(list_inst, (union list_value) element, DOUBLE_TYPE);
}

void list_add_ptr(list_t* list_inst, void* element)
//void list_add_float(list_t* list_inst, union list_value element)
{
    list_add(list_inst, (union list_value) element, DOUBLE_TYPE);
}



void print_list(list_t* list_inst)
{
    list_node_t *curr_node;
    curr_node = list_inst->head.next;

    printf("[");
    for(int i = 0; i < list_inst->size; i++)
    {
        switch(curr_node->element.type)
        {
            case INT64_TYPE:
                printf("%ld", curr_node->element.value.i);
                break;
            case DOUBLE_TYPE:
                printf("%f", curr_node->element.value.d);
                break;
            case PTR_TYPE:
                break;
            default:
                assert(1 && "Print not implemented for type");
                break;
        }
        if( i < list_inst->size - 1 )
        {
            printf(", ");
        }
        curr_node = curr_node->next;
    }
    printf("]\n");
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
