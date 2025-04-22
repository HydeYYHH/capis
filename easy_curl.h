#ifndef EASY_CURL_H
#define EASY_CURL_H

#include "read_yaml.h"

typedef struct {
    char *headers;        // Response headers
    size_t headers_size;  // Size of headers
    char *body;           // Response body
    size_t body_size;     // Size of body
    char **set_cookies;   // Array of Set-Cookie headers
    long status_code;     // HTTP status code
} Response;

// Free the memory allocated for a Response struct
void free_response(Response *resp);

// Perform an HTTP request with the given metadata and store the response
int do_easy_curl(METADATA *md, Response *resp, ...);

#endif