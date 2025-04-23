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

char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *new = malloc(len + 1);
    if (!new) return NULL;
    memcpy(new, s, len);
    new[len] = '\0';
    return new;
}

char **split_lines(const char *str) {
    if (!str) return NULL;

    size_t len = strlen(str);
    char *copy = strdup(str); // Copy so we can modify safely
    if (!copy) return NULL;

    char **lines = NULL;
    size_t count = 0;
    char *start = copy;

    for (size_t i = 0; i < len; i++) {
        if (copy[i] == '\r' || copy[i] == '\n') {
            copy[i] = '\0'; // Null-terminate this line

            // Only add if line is not empty
            if (*start != '\0') {
                char **tmp = realloc(lines, sizeof(char *) * (count + 1));
                if (!tmp) {
                    free(copy);
                    return NULL;
                }
                lines = tmp;
                lines[count++] = strdup(start); // Add line
            }

            // Handle \r\n together (Windows line ending)
            if (copy[i] == '\r' && i + 1 < len && copy[i + 1] == '\n') {
                i++; // skip the \n after \r
            }

            start = copy + i + 1; // Move to next potential line
        }
    }

    // Handle the final line if it doesn't end with \n or \r
    if (*start != '\0') {
        char **tmp = realloc(lines, sizeof(char *) * (count + 1));
        if (!tmp) {
            free(copy);
            return NULL;
        }
        lines = tmp;
        lines[count++] = strdup(start);
    }

    // NULL-terminate the array
    char **tmp = realloc(lines, sizeof(char *) * (count + 1));
    if (tmp) {
        lines = tmp;
        lines[count] = NULL;
    }

    free(copy);
    return lines;
}
