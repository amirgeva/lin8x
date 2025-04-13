#include <circular.h>
#include <vector.h>
#include <malloc.h>

struct _circular
{
	Vector *vector;
	uint size;
	uint write_index, read_index;
};

Circular *circular_new(uint size, uint element_size)
{
	Circular *circular = malloc(sizeof(Circular));
	if (!circular)
		return 0;

	circular->vector = vector_new(element_size);
	if (!circular->vector) {
		free(circular);
		return 0;
	}
	vector_resize(circular->vector, size);
	circular->size = size;
	circular->write_index = 0;
	circular->read_index = 0;

	return circular;
}

void circular_shut(Circular *circular)
{
	if (circular) {
		vector_shut(circular->vector);
		free(circular);
	}
}

uint circular_size(Circular *circular)
{
	if (!circular)
		return 0;

	return vector_size(circular->vector);
}

bool circular_empty(Circular *circular)
{
	if (!circular)
		return 0;

	return (circular->write_index == circular->read_index);
}

bool circular_full(Circular *circular)
{
	if (!circular)
		return 0;

	return (((circular->write_index + 1) % circular->size) == circular->read_index);
}
bool circular_read(Circular *circular, void *element)
{
	if (!circular || circular_empty(circular))
		return 0;

	if (vector_get(circular->vector, circular->read_index, element))
	{
		circular->read_index = (circular->read_index + 1) % circular->size;
		return 1;
	}
	return 0;
}

bool circular_write(Circular *circular, void *element)
{
	if (!circular || circular_full(circular))
		return 0;

	if (vector_set(circular->vector, circular->write_index, element))
	{
		circular->write_index = (circular->write_index + 1) % circular->size;
		return 1;
	}
	return 0;
}
