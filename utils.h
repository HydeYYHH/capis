#ifndef UTILS_H
#define UTILS_H

typedef struct Node {
    char *val;
    struct Node *next;
} Node;

typedef Node* StrLList;


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>

char *strndup(const char *s, size_t n);

StrLList init_strllist();
int ap_strllist(StrLList l, char *val);
void free_strllist(StrLList l);
char **split_lines(const char *str);

#endif
