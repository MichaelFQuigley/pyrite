/* small_set.h
 *
 *  Michael Quigley
 *
 *  Small set that maps string keys to pointer values.
 * Note: Uniqueness is currently not guaranteed. This is for performance
 *reasons.
 */

#ifndef SMALL_SET_H
#define SMALL_SET_H

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// SET_ARR_SIZE: size of underlying array
#define SET_ARR_SIZE 3

/*set_list_t:
 * Defines an element in the small set.
 */
typedef struct set_list_node {
  struct set_list_node* prev;
  struct set_list_node* next;

  char* key;
  void* value;
} set_list_node_t;

typedef struct {
  size_t size;
  set_list_node_t set_arr[SET_ARR_SIZE];
} small_set_t;

small_set_t* small_set_init(void);
void small_set_uninit(small_set_t* set);

void small_set_set(small_set_t* set, char* key, void* value);
void* small_set_get(small_set_t* set, char* key);
bool small_set_haskey(small_set_t* set, char* key);
bool small_set_remove(small_set_t* set, char* key);

#endif
