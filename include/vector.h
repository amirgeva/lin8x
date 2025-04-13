#pragma once

#include "types.h"

typedef struct vector_ Vector;

typedef void (*destructor_func)(void **);

Vector *vector_new(uint element_size);
void		vector_object_destructor(Vector* v, destructor_func destructor);
void		vector_shut(Vector*);
uint		vector_size(Vector*);
uint		vector_capacity(Vector*);
uint		vector_element_size(Vector*);
bool		vector_clear(Vector*);
bool		vector_resize(Vector*, uint size);
bool		vector_reserve(Vector*, uint size);
bool		vector_push(Vector*, void* element);
bool		vector_pop(Vector*, void* element);
bool		vector_insert(Vector*, uint index, void* element);
bool		vector_set(Vector* v, uint index, void* element);
bool		vector_get(Vector*, uint index, void* element);
void*		vector_access(Vector*, uint index);
bool		vector_erase(Vector*, uint index);
bool		vector_erase_range(Vector*, uint begin, uint end);
