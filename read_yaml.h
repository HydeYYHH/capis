#ifndef READ_YAML_H
#define READ_YAML_H

#include <stdbool.h>

enum CURL_METHOD {
    GET, POST, PUT, UPDATE, _DELETE
};

typedef struct {
    char *key;
    char *value;
} Header;

typedef struct {
    char *name;
    char *value;
    char *domain;
    char *path;
    char *expires;
    bool httpOnly;
    bool secure;
} Cookie;

typedef struct {
    char *key;
    char *value;
} Param;

typedef struct {
    enum CURL_METHOD method;
    char *host;
    char *path;
    char *url;
    long timeout;
    bool secure;
    Header *headers;
    Param *params;
    Cookie *cookies;
} METADATA;

// Free the memory allocated for a METADATA struct
void free_metadata(METADATA *md);

// Initialize a new METADATA struct
METADATA *init_metadata(void);

#include <stdio.h>
// Read YAML file and populate METADATA
int read_yaml(FILE *fp, METADATA *metadata);

// Print METADATA contents for debugging
void print_metadata(METADATA *metadata);

#endif