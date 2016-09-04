#include <stdio.h>
#include <assert.h>

#include "basic_types.h"

void list_tests()
{
    uint64_t size = 100;
    List* list = init_List(size);

    for(int64_t i = 0; i < size; i++)
    {
        Int* index = init_Int(i);
        Int* value = init_Int(i);
        set_List(list, index, value);
    }

    uint64_t new_size = size + 300;
    for(int64_t i = size; i < new_size; i++)
    {
        add_List(list, init_Int(i));
    }

    for(int64_t i = 0; i < new_size; i++)
    {
        printf("val = %ld\n", ((Int*)get_List(list, init_Int(i)))->raw_value);
    }
}


int main(void)
{
    list_tests();
    return 0;

}
