#pragma once

#include <types.h>

typedef struct _circular Circular;

/* Create a new circular buffer.  Buffer does not take ownership
   of dynamically allocated elements placed on it.  Such objects
   must be freed by the called */
Circular *circular_new(uint size, uint element_size);
void circular_shut(Circular *circular);
uint circular_size(Circular *circular);
bool circular_empty(Circular *circular);
bool circular_full(Circular *circular);
bool circular_read(Circular *circular, void *element);
bool circular_write(Circular *circular, void *element);
