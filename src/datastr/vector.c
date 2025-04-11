#include <vector.h>
#include <malloc.h>
//#include <utils.h>

void copy(void* dest, void* src, int size)
{
	if (size > 0)
	{
		char* cdst = (char*)dest;
		char* csrc = (char*)src;
		bool reverse = 0;
		if ((csrc < cdst && (csrc + size)>cdst) ||
			(cdst < csrc && (cdst + size)>csrc))
		{
			if (csrc < cdst) reverse = 1;
		}
		if (reverse)
		{
			for (int i = size-1; i >= 0; --i)
				cdst[i] = csrc[i];
		}
		else
		{
			for (int i = 0; i < size; ++i)
				cdst[i] = csrc[i];
		}
	}
}

struct vector_
{
	uint				element_size;
	uint				capacity;
	uint				size;
	void				(*destructor)(void**);
	char*				data;
};

void vector_object_destructor(Vector *v, void (*destructor)(void **))
{
	if (!v) return;
	v->destructor = destructor;
}

bool vector_init(Vector *v, uint element_size)
{
	if (!v)
		return 0;
	v->element_size = element_size;
	v->capacity = 0;
	v->size = 0;
	v->data = 0;
	v->destructor = 0;
	return 1;
}

Vector *vector_new(uint element_size)
{
	Vector* res = (Vector*)malloc(sizeof(Vector));
	if (!res) return 0;
	vector_init(res, element_size);
	return res;
}

void		vector_shut(Vector* v)
{
	vector_clear(v);
	if (v->data)
		free(v->data);
	if (v)
		free(v);
}

static bool vector_remalloc(Vector* v, uint new_size)
{
	if (!v) return 0;
	char* buffer = (char*)malloc(new_size * v->element_size);
	if (!buffer) return 0;
	if (v->data)
	{
		copy(buffer, v->data, (v->size * v->element_size));
		free(v->data);
	}
	v->data = buffer;
	v->capacity = new_size;
	return 1;
}

uint		vector_size(Vector* v)
{
	if (!v) return 0;
	return v->size;
}

bool		vector_clear(Vector* v)
{
	if (!v) return 0;
	vector_resize(v, 0);
	v->size = 0;
	return 1;
}

bool		vector_resize(Vector* v, uint size)
{
	if (!v) return 0;
	if (size < v->size && v->destructor)
	{
		for (uint i = size; i < v->size; ++i)
		{
			v->destructor((void**)(v->data + (i * v->element_size)));
		}
	}
	if (size > v->capacity)
	{
		if (!vector_remalloc(v, size)) return 0;
	}
	v->size = size;
	return 1;
}

bool		vector_reserve(Vector* v, uint size)
{
	if (!v) return 0;
	if (size <= v->capacity) return 1;
	return vector_remalloc(v, size);
}

bool ensure_capacity(Vector* v)
{
	if (v->size >= v->capacity)
	{
		uint new_size = 10;
		if (v->size > 0)
			new_size = v->size << 1;
		if (!vector_remalloc(v, new_size)) return 0;
	}
	return 1;
}

bool		vector_insert(Vector* v, uint index, void* element)
{
	if (!v) return 0;
	if (index >= v->size) return vector_push(v, element);
	ensure_capacity(v);
	char* dst = v->data + (index + 1) * v->element_size;
	char* src = dst - v->element_size;
	copy(dst, src, (v->size - index) * v->element_size);
	v->size++;
	vector_set(v, index, element);
	return 1;
}

bool		vector_push(Vector* v, void* element)
{
	if (!v) return 0;
	ensure_capacity(v);
	copy(v->data + (v->size * v->element_size), element, v->element_size);
	v->size++;
	return 1;
}

bool		vector_pop(Vector* v, void* element)
{
	if (!v) return 0;
	if (v->size > 0)
	{
		v->size--;
		if (element)
			copy(element, v->data + (v->size * v->element_size), v->element_size);
		else if (v->destructor)
		{
			v->destructor((void**)(v->data + (v->size * v->element_size)));
		}
		return 1;
	}
	return 0;
}

void*		vector_access(Vector* v, uint index)
{
	if (!v) return 0;
	if (index >= v->size) return 0;
	return v->data + (index * v->element_size);
}

bool		vector_set(Vector* v, uint index, void* element)
{
	if (v)
	{
		void* dst = vector_access(v, index);
		if (element && dst)
		{
			if (v->destructor)
				v->destructor(dst);
			copy(dst, element, v->element_size);
			return 1;
		}
	}
	return 0;
}

bool		vector_get(Vector* v, uint index, void* element)
{
	if (!v) return 0;
	void* src = vector_access(v, index);
	if (element && src)
	{
		copy(element, src, v->element_size);
		return 1;
	}
	return 0;
}

bool		vector_erase(Vector* v, uint index)
{
	if (!v) return 0;
	if (index >= v->size) return 0;
	if (v->destructor)
	{
		v->destructor((void**)(v->data + (index * v->element_size)));
	}
	if (index < (v->size - 1))
	{
		copy(vector_access(v, index),
			 vector_access(v, index + 1),
			 (v->element_size * (v->size - index - 1)));
	}
	v->size--;
	return 1;
}

bool		vector_erase_range(Vector* v, uint begin, uint end)
{
	if (!v) return 0;
	if (begin >= v->size || end>v->size || begin>=end) return 0;
	uint n=end-begin;
	if (v->destructor)
	{
		for (uint i = begin; i < end; ++i)
		{
			v->destructor((void**)(v->data + (i * v->element_size)));
		}
	}
	if (end < v->size)
	{
		copy(vector_access(v, begin), 
			 vector_access(v, end), 
			 (v->element_size * (v->size - end)));
	}
	v->size -= n;
	return 1;
}

uint		vector_capacity(Vector* v)
{
	if (!v) return 0;
	return v->capacity;
}

uint		vector_element_size(Vector* v)
{
	if (!v) return 0;
	return v->element_size;
}

