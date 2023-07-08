#ifndef __VECTOR_H
#define __VECTOR_H


typedef struct vector_string {
    char **data;
    int size;
    int capacity;
}vector_string;

vector_string* vector_string_init(int capacity);

void vector_string_push_back(struct vector_string *v, char *s);

void vector_string_free(struct vector_string *v);

#endif
