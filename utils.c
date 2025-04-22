#include "utils.h"
#include <stdlib.h>
#include <string.h>

StrLList init_strllist() {
    StrLList l = malloc(sizeof(Node));
    if (!l) return NULL;
    l->val = NULL;
    l->next = NULL;
    return l;
}

int ap_strllist(StrLList l, char *val) {
    if (!l) return -1;
    Node *node = malloc(sizeof(Node));
    if (!node) return -1;

    node->val = strdup(val);
    if (!node->val) {
        free(node);
        return -1;
    }
    node->next = NULL;

    Node *cur = l;
    while (cur->next) cur = cur->next;
    cur->next = node;

    return 0;
}

void free_strllist(StrLList l) {
    if (!l) return;

    Node *cur = l;
    while (cur) {
        Node *next = cur->next;
        if (cur->val) free(cur->val);
        free(cur);
        cur = next;
    }
}
