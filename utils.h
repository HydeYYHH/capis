#ifndef UTILS_H
#define UTILS_H

typedef struct Node {
    char *val;
    struct Node *next;
} Node;

typedef Node* StrLList;

StrLList init_strllist();
int ap_strllist(StrLList l, char *val);
void free_strllist(StrLList l);

#endif
