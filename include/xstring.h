#pragma once

#include <vector.h>

typedef struct string_ String;


String* string_new(const char* text); // Can be null
String* string_nnew(const char* text, uint n);
void string_shut(String* s);
void string_clear(String* s);
void string_set(String* s, const char* text); // Can be null
void string_append(String* s, const char* text); // Can be null
String* string_substring(String* s, int start, int n);
const char* string_get(String* s);
void string_copy(String* dst, String* src);
int string_compare(String* s1, String* s2);
int string_length(String* s);
void string_resize(String* s, int size);
int string_find(String* s, const char* text);
int string_find_char(String* s, char c);
int string_find_last_char(String* s, char c);

Vector* string_split(String* s, const char* delimiters);


