#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/vector.h"

#define ASSERT(condition, message) \
    if (!(condition)) { \
        printf("Assertion failed: %s, file: %s, line: %d\n", message, __FILE__, __LINE__); \
        return 1; \
    }

int test_vector_new() {
    Vector* v = vector_new(sizeof(int));
    ASSERT(v, "vector_new failed to allocate memory");
    vector_shut(v);
    return 0;
}

int test_vector_size() {
    Vector* v = vector_new(sizeof(int));
    ASSERT(vector_size(v) == 0, "Initial size should be 0");
    vector_shut(v);
    return 0;
}

int test_vector_capacity() {
    Vector* v = vector_new(sizeof(int));
    ASSERT(vector_capacity(v) == 0, "Initial capacity should be 0");
    vector_shut(v);
    return 0;
}

int test_vector_element_size() {
    Vector* v = vector_new(sizeof(int));
    ASSERT(vector_element_size(v) == sizeof(int), "Element size should match");
    vector_shut(v);
    return 0;
}

int test_vector_push_pop() {
    Vector* v = vector_new(sizeof(int));
    int value1 = 10;
    int value2 = 20;

    ASSERT(vector_push(v, &value1), "vector_push failed");
    ASSERT(vector_size(v) == 1, "Size should be 1 after push");

    ASSERT(vector_push(v, &value2), "vector_push failed");
    ASSERT(vector_size(v) == 2, "Size should be 2 after push");

    int popped_value;
    ASSERT(vector_pop(v, &popped_value), "vector_pop failed");
    ASSERT(vector_size(v) == 1, "Size should be 1 after pop");
    ASSERT(popped_value == value2, "Popped value should be correct");

    ASSERT(vector_pop(v, &popped_value), "vector_pop failed");
    ASSERT(vector_size(v) == 0, "Size should be 0 after pop");
    ASSERT(popped_value == value1, "Popped value should be correct");

    vector_shut(v);
    return 0;
}

int test_vector_access() {
    Vector* v = vector_new(sizeof(int));
    int value1 = 10;
    int value2 = 20;

    vector_push(v, &value1);
    vector_push(v, &value2);

    int* ptr1 = (int*)vector_access(v, 0);
    ASSERT(ptr1, "vector_access returned NULL");
    ASSERT(*ptr1 == value1, "Accessed value is incorrect");

    int* ptr2 = (int*)vector_access(v, 1);
    ASSERT(ptr2, "vector_access returned NULL");
    ASSERT(*ptr2 == value2, "Accessed value is incorrect");

    vector_shut(v);
    return 0;
}

int test_vector_set_get() {
    Vector* v = vector_new(sizeof(int));
    int value1 = 10;
    int value2 = 20;
	int value3 = 30;

    vector_push(v, &value1);
    vector_push(v, &value2);

	ASSERT(vector_set(v, 0, &value3), "vector_set failed");
    int retrieved_value;
    ASSERT(vector_get(v, 0, &retrieved_value), "vector_get failed");
    ASSERT(retrieved_value == value3, "Retrieved value after set is incorrect");
	ASSERT(vector_get(v, 1, &retrieved_value), "vector_get failed");
    ASSERT(retrieved_value == value2, "Retrieved value after set is incorrect");

    vector_shut(v);
    return 0;
}

int test_vector_insert() {
    Vector* v = vector_new(sizeof(int));
    int value1 = 10;
    int value2 = 20;
    int value3 = 30;

    vector_push(v, &value1);
    vector_push(v, &value2);

    ASSERT(vector_insert(v, 1, &value3), "vector_insert failed");
    ASSERT(vector_size(v) == 3, "Size should be 3 after insert");

    int retrieved_value;
    vector_get(v, 0, &retrieved_value);
    ASSERT(retrieved_value == value1, "Value at index 0 is incorrect");
    vector_get(v, 1, &retrieved_value);
    ASSERT(retrieved_value == value3, "Value at index 1 is incorrect");
    vector_get(v, 2, &retrieved_value);
    ASSERT(retrieved_value == value2, "Value at index 2 is incorrect");

    vector_shut(v);
    return 0;
}

int test_vector_erase() {
    Vector* v = vector_new(sizeof(int));
    int value1 = 10;
    int value2 = 20;
    int value3 = 30;

    vector_push(v, &value1);
    vector_push(v, &value2);
    vector_push(v, &value3);

    ASSERT(vector_erase(v, 1), "vector_erase failed");
    ASSERT(vector_size(v) == 2, "Size should be 2 after erase");

    int retrieved_value;
    vector_get(v, 0, &retrieved_value);
    ASSERT(retrieved_value == value1, "Value at index 0 is incorrect");
    vector_get(v, 1, &retrieved_value);
    ASSERT(retrieved_value == value3, "Value at index 1 is incorrect");

    vector_shut(v);
    return 0;
}

int test_vector_erase_range() {
    Vector* v = vector_new(sizeof(int));
    int value1 = 10;
    int value2 = 20;
    int value3 = 30;
    int value4 = 40;

    vector_push(v, &value1);
    vector_push(v, &value2);
    vector_push(v, &value3);
    vector_push(v, &value4);

    ASSERT(vector_erase_range(v, 1, 3), "vector_erase_range failed");
    ASSERT(vector_size(v) == 2, "Size should be 2 after erase_range");

    int retrieved_value;
    vector_get(v, 0, &retrieved_value);
    ASSERT(retrieved_value == value1, "Value at index 0 is incorrect");
    vector_get(v, 1, &retrieved_value);
    ASSERT(retrieved_value == value4, "Value at index 1 is incorrect");

    vector_shut(v);
    return 0;
}

int test_vector_clear() {
    Vector* v = vector_new(sizeof(int));
    int value1 = 10;
    int value2 = 20;

    vector_push(v, &value1);
    vector_push(v, &value2);

    ASSERT(vector_clear(v), "vector_clear failed");
    ASSERT(vector_size(v) == 0, "Size should be 0 after clear");

    vector_shut(v);
    return 0;
}

int test_vector_resize() {
    Vector* v = vector_new(sizeof(int));
    
    ASSERT(vector_resize(v, 5), "vector_resize failed");
    ASSERT(vector_size(v) == 5, "Size should be 5 after resize");
    ASSERT(vector_capacity(v) >= 5, "Capacity should be at least 5 after resize");

    ASSERT(vector_resize(v, 0), "vector_resize failed");
    ASSERT(vector_size(v) == 0, "Size should be 0 after resize");

    vector_shut(v);
    return 0;
}

int test_vector_reserve() {
    Vector* v = vector_new(sizeof(int));

    ASSERT(vector_reserve(v, 10), "vector_reserve failed");
    ASSERT(vector_capacity(v) >= 10, "Capacity should be at least 10 after reserve");

    vector_shut(v);
    return 0;
}

int main() {
    int failures = 0;
    failures += test_vector_new();
    failures += test_vector_size();
    failures += test_vector_capacity();
    failures += test_vector_element_size();
    failures += test_vector_push_pop();
    failures += test_vector_access();
    failures += test_vector_set_get();
    failures += test_vector_insert();
    failures += test_vector_erase();
    failures += test_vector_erase_range();
    failures += test_vector_clear();
    failures += test_vector_resize();
    failures += test_vector_reserve();

    if (failures == 0) {
        printf("All tests passed!\n");
    } else {
        printf("%d tests failed.\n", failures);
    }

    return (failures > 0);
}

