#include <xstring.h>
#include <stdio.h>
#include <string.h>

void test_string_new()
{
	String *str = string_new("hello");
	if (!str)
	{
		printf("test_string_new failed: string_new returned NULL\n");
		return;
	}
	const char *text = string_get(str);
	if (!text)
	{
		printf("test_string_new failed: string_get returned NULL\n");
	}
	else if (strcmp(text, "hello") != 0)
	{
		printf("test_string_new failed: expected 'hello', got '%s'\n", text);
	}
	string_shut(str);
}

void test_string_nnew()
{
	String *str = string_nnew("hello world", 5);
	if (!str)
	{
		printf("test_string_nnew failed: string_nnew returned NULL\n");
		return;
	}
	const char *text = string_get(str);
	if (!text)
	{
		printf("test_string_nnew failed: string_get returned NULL\n");
	}
	else if (strcmp(text, "hello") != 0)
	{
		printf("test_string_nnew failed: expected 'hello', got '%s'\n", text);
	}
	string_shut(str);
}

void test_string_shut()
{
	String *str = string_new("test");
	string_shut(str);
	// How to properly test shut?  Valgrind?  For now, assume it works if no crash.
}

void test_string_clear()
{
	String *str = string_new("test");
	string_clear(str);
	if (string_length(str) != 0)
	{
		printf("test_string_clear failed: length should be 0\n");
	}
	string_shut(str);
}

void test_string_set()
{
	String *str = string_new("initial");
	string_set(str, "new value");
	const char *text = string_get(str);
	if (strcmp(text, "new value") != 0)
	{
		printf("test_string_set failed: expected 'new value', got '%s'\n", text);
	}
	string_shut(str);
}

void test_string_append()
{
	String *str = string_new("hello");
	string_append(str, " world");
	const char *text = string_get(str);
	if (strcmp(text, "hello world") != 0)
	{
		printf("test_string_append failed: expected 'hello world', got '%s'\n", text);
	}
	string_shut(str);
}

void test_string_get()
{
	String *str = string_new("getter test");
	const char *text = string_get(str);
	if (strcmp(text, "getter test") != 0)
	{
		printf("test_string_get failed: expected 'getter test', got '%s'\n", text);
	}
	string_shut(str);
}

void test_string_copy()
{
	String *src = string_new("source");
	String *dest = string_new("destination");
	string_copy(dest, src);
	const char *text = string_get(dest);
	if (strcmp(text, "source") != 0)
	{
		printf("test_string_copy failed: expected 'source', got '%s'\n", text);
	}
	string_shut(src);
	string_shut(dest);
}

void test_string_compare()
{
	String *str1 = string_new("abc");
	String *str2 = string_new("abc");
	String *str3 = string_new("abd");
	if (string_compare(str1, str2) != 0)
	{
		printf("test_string_compare failed: expected 0\n");
	}
	if (string_compare(str1, str3) >= 0)
	{
		printf("test_string_compare failed: expected negative\n");
	}
	string_shut(str1);
	string_shut(str2);
	string_shut(str3);
}

void test_string_length()
{
	String *str = string_new("length");
	if (string_length(str) != 6)
	{
		printf("test_string_length failed: expected 6, got %d\n", string_length(str));
	}
	string_shut(str);
}

void test_string_resize()
{
	String *str = string_new("small");
	string_resize(str, 10);
	if (string_length(str) != 10)
	{
		printf("test_string_resize failed: length should still be 5\n");
	}
	string_resize(str, 2);
	if (string_length(str) != 2)
	{
		printf("test_string_resize failed: length should be 2\n");
	}
	string_shut(str);
}

void test_string_find()
{
	String *str = string_new("hello world");
	int index = string_find(str, "world");
	if (index != 6)
	{
		printf("test_string_find failed: expected 6, got %d\n", index);
	}
	string_shut(str);
}

void test_string_find_char()
{
	String *str = string_new("hello world");
	int index = string_find_char(str, 'o');
	if (index != 4)
	{
		printf("test_string_find_char failed: expected 4, got %d\n", index);
	}
	string_shut(str);
}

void test_string_find_last_char()
{
	String *str = string_new("hello world");
	int index = string_find_last_char(str, 'o');
	if (index != 7)
	{
		printf("test_string_find_last_char failed: expected 7, got %d\n", index);
	}
	string_shut(str);
}

void test_string_split()
{
	String *str = string_new("hello,world,test");
	Vector *parts = string_split(str, ",");
	if (vector_size(parts) != 3)
	{
		printf("test_string_split failed: expected 3 parts, got %d\n", vector_size(parts));
	}
	else
	{
		String *part1 = *(String **)vector_access(parts, 0);
		String *part2 = *(String **)vector_access(parts, 1);
		String *part3 = *(String **)vector_access(parts, 2);
		if (strcmp(string_get(part1), "hello") != 0)
		{
			printf("test_string_split failed: part1 expected 'hello', got '%s'\n", string_get(part1));
		}
		if (strcmp(string_get(part2), "world") != 0)
		{
			printf("test_string_split failed: part2 expected 'world', got '%s'\n", string_get(part2));
		}
		if (strcmp(string_get(part3), "test") != 0)
		{
			printf("test_string_split failed: part3 expected 'test', got '%s'\n", string_get(part3));
		}
	}
	vector_shut(parts);
	string_shut(str);
}

int main(int argc, char *argv[])
{
	test_string_new();
	test_string_nnew();
	test_string_clear();
	test_string_set();
	test_string_append();
	test_string_get();
	test_string_copy();
	test_string_compare();
	test_string_length();
	test_string_resize();
	test_string_find();
	test_string_find_char();
	test_string_find_last_char();
	test_string_split();
	return 0;
}
