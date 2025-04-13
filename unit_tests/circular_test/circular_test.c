#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <circular.h>

#define ASSERT(condition, message) \
    if (!(condition)) { \
        printf("Assertion failed: %s, file: %s, line: %d\n", message, __FILE__, __LINE__); \
        return 1; \
    }

int test_circular_new() {
    Circular* c = circular_new(5, sizeof(int));
    ASSERT(c, "circular_new failed to allocate memory");
    circular_shut(c);
    return 0;
}

int test_circular_size() {
    Circular* c = circular_new(5, sizeof(int));
    ASSERT(circular_size(c) == 5, "circular_size is incorrect");
    circular_shut(c);
    return 0;
}

int test_circular_empty() {
    Circular* c = circular_new(5, sizeof(int));
    ASSERT(circular_empty(c) == 1, "circular_empty should return true for a new circular buffer");
    circular_shut(c);
    return 0;
}

int test_circular_full() {
    Circular* c = circular_new(2, sizeof(int));
    int value = 10;
    circular_write(c, &value);
    ASSERT(circular_full(c) == 1, "circular_full should return true");
    circular_shut(c);
    return 0;
}

int test_circular_write_read() {
    Circular* c = circular_new(5, sizeof(int));
    int value1 = 10;
    int value2 = 20;

    ASSERT(circular_write(c, &value1), "circular_write failed");
    ASSERT(circular_empty(c) == 0, "circular_empty should return false after write");
    ASSERT(circular_full(c) == 0, "circular_full should return false after write");

    ASSERT(circular_write(c, &value2), "circular_write failed");

    int read_value1;
    ASSERT(circular_read(c, &read_value1), "circular_read failed");
    ASSERT(read_value1 == value1, "Read value is incorrect");

    int read_value2;
    ASSERT(circular_read(c, &read_value2), "circular_read failed");
    ASSERT(read_value2 == value2, "Read value is incorrect");
    ASSERT(circular_empty(c) == 1, "circular_empty should return true after reading all elements");

    circular_shut(c);
    return 0;
}

int main(int argc, char* argv[]) {
    int failures = 0;
    failures += test_circular_new();
    failures += test_circular_size();
    failures += test_circular_empty();
    failures += test_circular_full();
    failures += test_circular_write_read();

    if (failures == 0) {
        printf("All circular buffer tests passed!\n");
    } else {
        printf("%d circular buffer tests failed.\n", failures);
    }

    return (failures > 0);
}

