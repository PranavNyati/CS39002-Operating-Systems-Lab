#include "vector.h"
#include <stdlib.h>
#include <string.h>

vector_string *vector_string_init(int capacity)
{
    
    vector_string *v = (vector_string *)malloc(sizeof(vector_string));
    v->capacity = capacity;
    v->size = 0;
    v->data = (char **)malloc(sizeof(char *) * capacity);
    return v;
}

void vector_string_push_back(struct vector_string *v, char *s)
{
    if (v->size == v->capacity)
    {
        v->capacity *= 2;
        v->data = (char **)realloc(v->data, sizeof(char *) * (v->capacity) );
    }
    v->data[v->size] = (char *)malloc(sizeof(char) * (strlen(s) + 1));
    strcpy(v->data[v->size], s);
    v->size++;
}

void vector_string_free(vector_string *v)
{
    for (int i = 0; i < v->size; i++)
    {
        free(v->data[i]);
    }
    free(v->data);
    free(v);
}
