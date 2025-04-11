#include <vector.h>
#include <xstring.h>
#include <malloc.h>
#include <string.h>

struct string_
{
	Vector *vec;
};

String* string_new(const char *text)
{
	String* s = (String*)malloc(sizeof(String));
	if (!s)
		return 0;

	s->vec = vector_new(sizeof(char));
	if (!s->vec)
	{
		free(s);
		return 0;
	}

	if (text)
	{
		string_append(s, text);
	}
	return s;
}

String *string_nnew(const char *text, uint n)
{
	String* s = (String*)malloc(sizeof(String));
	if (!s)
		return 0;

	s->vec = vector_new(sizeof(char));
	if (!s->vec)
	{
		free(s);
		return 0;
	}

	if (text && n>0)
	{
		int len = strlen(text);
		if (len > n)
			len = n;
		vector_resize(s->vec, len + 1);
		char* dst = vector_access(s->vec, 0);
		for(int i = 0; i < len; ++i)
		{
			dst[i] = text[i];
		}
		dst[len] = 0;
	}
	return s;
}

void string_shut(String* s)
{
	if (s)
	{
		vector_shut(s->vec);
		free(s);
	}
}

void string_destructor(void** pstr)
{
	if (pstr)
	{
		String* s = (String*)*pstr;
		if (s)
		{
			string_shut(s);
		}
	}
}


void string_clear(String *s)
{
	if (s)
	{
		vector_clear(s->vec);
	}
}

String *string_substring(String *s, int start, int n)
{
	if (!s || n <= 0)
		return 0;

	int len = string_length(s);
	if (start < 0 || start >= len)
		return 0;
	int limit = len - start;
	if (n > limit)
		n = limit;

	return string_nnew(vector_access(s->vec, start), n);
}

void string_trim(String* s)
{
	if (s)
	{
		int len = string_length(s);
		int start = -1, stop = len;
		const char* str = vector_access(s->vec, 0);
		for (; start < len;++start)
		{
			if (str[start] != ' ' && str[start] != '\t')
				break;
		}
		for (; stop > start;--stop)
		{
			if (str[stop - 1] != ' ' && str[stop - 1] != '\t')
				break;
		}
		if (start >= stop)
		{
			string_clear(s);
		}
		else
		{
			vector_erase_range(s->vec, stop, len);
			vector_erase_range(s->vec, 0, start);
			char* ptr=vector_access(s->vec, stop - start);
			*ptr = 0;
		}
	}
}

void string_set(String* s, const char *text)
{
	string_clear(s);
	string_append(s, text);
}

void string_append(String* s, const char *text)
{
	if (s && text)
	{
		int len = strlen(text);
		uint n = vector_size(s->vec);
		if (n > 0)
		{
			vector_pop(s->vec, 0);
			--n;
		}
		vector_resize(s->vec, n + len + 1);
		strcpy(vector_access(s->vec, n), text);
	}
}

const char *string_get(String* s)
{
	if (s)
	{
		return vector_access(s->vec, 0);
	}
	return 0;
}

void string_copy(String* dst, String* src)
{
	if (dst && src)
	{
		string_clear(dst);
		int len = string_length(src);
		vector_resize(dst->vec, len + 1);
		strcpy(vector_access(dst->vec, 0), vector_access(src->vec, 0));
	}
}

int string_compare(String* s1, String* s2)
{
	if (s1 && s2)
	{
		return strcmp(vector_access(s1->vec, 0), vector_access(s2->vec, 0));
	}
	return -1;
}

int string_length(String* s)
{
	if (s)
	{
		uint n = vector_size(s->vec);
		if (n==0)
			return 0;
		return n-1;
	}
	return -1;
}

int string_find(String* s, const char* text)
{
	if (s && text)
	{
		const char* str = vector_access(s->vec, 0);
		const char* found = strstr(str, text);
		if (found)
		{
			return found - str;
		}
	}
	return -1;
}

int string_find_char(String* s, char c)
{
	if (s)
	{
		const char* str = vector_access(s->vec, 0);
		const char* found = strchr(str, c);
		if (found)
		{
			return found - str;
		}
	}
	return -1;
}

int string_find_last_char(String* s, char c)
{
	if (s)
	{
		const char* str = vector_access(s->vec, 0);
		const char* found = strrchr(str, c);
		if (found)
		{
			return found - str;
		}
	}
	return -1;
}

void string_resize(String* s, int size)
{
	if (s)
	{
		vector_resize(s->vec, size + 1);
		char* str = vector_access(s->vec, 0);
	}
}

Vector *string_split(String *s, const char *delimiters)
{
	if (!s || !delimiters)
		return 0;

	Vector *result = vector_new(sizeof(String*));
	vector_object_destructor(result, (void (*)(void**))string_destructor);
	if (!result)
		return 0;
	bool delim_lut[256];
	for(int i = 0; i < 256; ++i)
		delim_lut[i] = 0;
	for (const char* d = delimiters; *d; ++d)
	{
		delim_lut[(unsigned char)*d] = 1;
	}
	char *start = 0;
	char *str = vector_access(s->vec, 0);
	while (*str)
	{
		if (delim_lut[(unsigned char)*str])
		{
			if (start)
			{
				String* token = string_nnew(start, str - start);
				vector_push(result, &token);
				start = 0;
			}
			str++;
			continue;
		}
		else
		{
			if (!start)
				start = str;
			str++;
		}
	}
	if (start)
	{
		String* token = string_nnew(start, str - start);
		vector_push(result, &token);
	}
	return result;
}


