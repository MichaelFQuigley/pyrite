//List implementation
////Author: Michael Quigley

union list_value
{
    int64_t i;
    int8_t  b;
    double   d;
    void*    v;
};

typedef enum list_element_types
{
    INT64_TYPE,
    DOUBLE_TYPE,
    PTR_TYPE,
    BOOL_TYPE
} list_element_types_t;

typedef struct list_element
{
    list_element_types_t type;
    union list_value   value;
} list_element_t;

typedef struct list_node
{
    struct list_node *next;
    struct list_node *prev;
    list_element_t element;
} list_node_t;

typedef struct list
{
   int size;
   list_node_t head; 
   list_node_t tail; 
} list_t;

list_t* list_init();
void list_uninit(list_t* list_inst);
void print_list(list_t* list_inst);

//void list_add_int(list_t* list_inst, union list_value element);
//void list_add_float(list_t* list_inst, union list_value element);
void list_add_int(list_t* list_inst, int64_t element);
void list_add_float(list_t* list_inst, double element);
void list_add_ptr(list_t* list_inst, void* element);
void list_add_bool(list_t* list_inst, int8_t element);
