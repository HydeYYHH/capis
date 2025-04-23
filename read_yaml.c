#include "read_yaml.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <yaml.h>

// Convert string to lowercase
static void to_lowercase(char *str) {
    for (; *str; ++str) *str = tolower(*str);
}

// Free memory allocated for METADATA struct
void free_metadata(METADATA *md) {
    if (!md) return;

    free(md->host);
    free(md->path);
    free(md->url);

    if (md->headers) {
        for (Header *h = md->headers; h->key != NULL; h++) {
            free(h->key);
            free(h->value);
        }
        free(md->headers);
    }

    if (md->params) {
        for (Param *p = md->params; p->key != NULL; p++) {
            free(p->key);
            free(p->value);
        }
        free(md->params);
    }

    if (md->cookies) {
        for (Cookie *c = md->cookies; c->name != NULL; c++) {
            free(c->name);
            free(c->value);
            free(c->domain);
            free(c->path);
            free(c->expires);
        }
        free(md->cookies);
    }

    free(md);
}

// Initialize a new METADATA struct with default values
METADATA *init_metadata(void) {
    METADATA *meta = malloc(sizeof(METADATA));
    if (!meta) {
        LOG_ERROR("Failed to allocate METADATA");
        return NULL;
    }

    meta->method = GET;
    meta->host = strdup("localhost");
    meta->path = strdup("/");
    meta->url = strdup("");
    meta->timeout = 0;
    meta->secure = true; // Default to secure (SSL enabled)
    meta->headers = NULL;
    meta->params = NULL;
    meta->cookies = NULL;

    if (!meta->host || !meta->path || !meta->url) {
        LOG_ERROR("Failed to allocate strings in init_metadata");
        free_metadata(meta);
        return NULL;
    }

    return meta;
}

// Parse HTTP method from string
static enum CURL_METHOD parse_method(const char *method) {
    if (strcmp(method, "GET") == 0) return GET;
    if (strcmp(method, "POST") == 0) return POST;
    if (strcmp(method, "PUT") == 0) return PUT;
    if (strcmp(method, "UPDATE") == 0) return UPDATE;
    if (strcmp(method, "DELETE") == 0) return _DELETE;
    return GET;
}

// Read YAML file and populate METADATA
int read_yaml(FILE *fp, METADATA *meta) {
    yaml_parser_t parser;
    yaml_event_t event;

    if (!yaml_parser_initialize(&parser)) {
        LOG_ERROR("Failed to initialize YAML parser");
        return -1;
    }

    yaml_parser_set_input_file(&parser, fp);

    int done = 0;
    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            LOG_ERROR("YAML parsing failed");
            break;
        }

        switch (event.type) {
            case YAML_STREAM_START_EVENT:
                break;
            case YAML_DOCUMENT_START_EVENT:
                break;
            case YAML_MAPPING_START_EVENT:
                while (1) {
                    if (!yaml_parser_parse(&parser, &event)) {
                        break;
                    }
                    if (event.type == YAML_MAPPING_END_EVENT) {
                        yaml_event_delete(&event);
                        break;
                    }
                    if (event.type == YAML_SCALAR_EVENT) {
                        char *key = strdup((char*)event.data.scalar.value);
                        if (!key) {
                            LOG_ERROR("Failed to allocate key string");
                            yaml_event_delete(&event);
                            break;
                        }
                        to_lowercase(key);
                        yaml_event_delete(&event);
                        if (!yaml_parser_parse(&parser, &event)) {
                            free(key);
                            break;
                        }

                        if (strcmp(key, "method") == 0 && event.type == YAML_SCALAR_EVENT) {
                            meta->method = parse_method((char*)event.data.scalar.value);
                        } else if (strcmp(key, "host") == 0 && event.type == YAML_SCALAR_EVENT) {
                            free(meta->host);
                            meta->host = strdup((char*)event.data.scalar.value);
                            if (!meta->host) LOG_ERROR("Failed to allocate host string");
                        } else if (strcmp(key, "path") == 0 && event.type == YAML_SCALAR_EVENT) {
                            free(meta->path);
                            meta->path = strdup((char*)event.data.scalar.value);
                            if (!meta->path) LOG_ERROR("Failed to allocate path string");
                        } else if (strcmp(key, "url") == 0 && event.type == YAML_SCALAR_EVENT) {
                            free(meta->url);
                            meta->url = strdup((char*)event.data.scalar.value);
                            if (!meta->url) LOG_ERROR("Failed to allocate url string");
                        } else if (strcmp(key, "timeout") == 0 && event.type == YAML_SCALAR_EVENT) {
                            meta->timeout = strtol((char*)event.data.scalar.value, NULL, 10);
                        } else if (strcmp(key, "secure") == 0 && event.type == YAML_SCALAR_EVENT) {
                            meta->secure = strcmp((char*)event.data.scalar.value, "true") == 0;
                        } else if (strcmp(key, "headers") == 0) {
                            if (event.type == YAML_SEQUENCE_START_EVENT) {
                                // Parse headers as a sequence of key-value mappings
                                yaml_event_delete(&event);
                                Header *headers = NULL;
                                int count = 0;
                                while (1) {
                                    if (!yaml_parser_parse(&parser, &event)) {
                                        break;
                                    }
                                    if (event.type == YAML_SEQUENCE_END_EVENT) {
                                        yaml_event_delete(&event);
                                        break;
                                    }
                                    if (event.type == YAML_MAPPING_START_EVENT) {
                                        yaml_event_delete(&event);
                                        char *header_key = NULL;
                                        char *header_value = NULL;
                                        while (1) {
                                            if (!yaml_parser_parse(&parser, &event)) {
                                                break;
                                            }
                                            if (event.type == YAML_MAPPING_END_EVENT) {
                                                yaml_event_delete(&event);
                                                break;
                                            }
                                            if (event.type == YAML_SCALAR_EVENT) {
                                                char *map_key = strdup((char*)event.data.scalar.value);
                                                if (!map_key) {
                                                    LOG_ERROR("Failed to allocate map_key string");
                                                    yaml_event_delete(&event);
                                                    break;
                                                }
                                                to_lowercase(map_key);
                                                yaml_event_delete(&event);
                                                if (!yaml_parser_parse(&parser, &event)) {
                                                    free(map_key);
                                                    break;
                                                }
                                                if (event.type == YAML_SCALAR_EVENT) {
                                                    if (strcmp(map_key, "key") == 0) {
                                                        header_key = strdup((char*)event.data.scalar.value);
                                                        if (!header_key) LOG_ERROR("Failed to allocate header_key");
                                                    } else if (strcmp(map_key, "value") == 0) {
                                                        header_value = strdup((char*)event.data.scalar.value);
                                                        if (!header_value) LOG_ERROR("Failed to allocate header_value");
                                                    }
                                                    yaml_event_delete(&event);
                                                } else {
                                                    yaml_event_delete(&event);
                                                }
                                                free(map_key);
                                            } else {
                                                yaml_event_delete(&event);
                                            }
                                        }
                                        if (header_key && header_value) {
                                            Header *temp = realloc(headers, (count + 1) * sizeof(Header));
                                            if (temp) {
                                                headers = temp;
                                                headers[count].key = header_key;
                                                headers[count].value = header_value;
                                                count++;
                                            } else {
                                                free(header_key);
                                                free(header_value);
                                                LOG_ERROR("Failed to reallocate headers array");
                                            }
                                        } else {
                                            free(header_key);
                                            free(header_value);
                                        }
                                    } else {
                                        yaml_event_delete(&event);
                                    }
                                }
                                if (headers) {
                                    Header *temp = realloc(headers, (count + 1) * sizeof(Header));
                                    if (temp) {
                                        headers = temp;
                                        headers[count].key = NULL;
                                        headers[count].value = NULL;
                                        meta->headers = headers;
                                    } else {
                                        LOG_ERROR("Failed to reallocate headers array");
                                        for (int i = 0; i < count; i++) {
                                            free(headers[i].key);
                                            free(headers[i].value);
                                        }
                                        free(headers);
                                    }
                                }
                            } else if (event.type == YAML_MAPPING_START_EVENT) {
                                // Parse headers as direct key-value pairs
                                yaml_event_delete(&event);
                                Header *headers = NULL;
                                int count = 0;
                                while (1) {
                                    if (!yaml_parser_parse(&parser, &event)) {
                                        break;
                                    }
                                    if (event.type == YAML_MAPPING_END_EVENT) {
                                        yaml_event_delete(&event);
                                        break;
                                    }
                                    if (event.type == YAML_SCALAR_EVENT) {
                                        char *header_key = strdup((char*)event.data.scalar.value);
                                        if (!header_key) {
                                            LOG_ERROR("Failed to allocate header_key string");
                                            yaml_event_delete(&event);
                                            continue;
                                        }
                                        yaml_event_delete(&event);
                                        if (!yaml_parser_parse(&parser, &event)) {
                                            free(header_key);
                                            break;
                                        }
                                        if (event.type == YAML_SCALAR_EVENT) {
                                            char *header_value = strdup((char*)event.data.scalar.value);
                                            if (!header_value) {
                                                LOG_ERROR("Failed to allocate header_value string");
                                                free(header_key);
                                            } else {
                                                Header *temp = realloc(headers, (count + 1) * sizeof(Header));
                                                if (temp) {
                                                    headers = temp;
                                                    headers[count].key = header_key;
                                                    headers[count].value = header_value;
                                                    count++;
                                                } else {
                                                    free(header_key);
                                                    free(header_value);
                                                    LOG_ERROR("Failed to reallocate headers array");
                                                }
                                            }
                                        } else {
                                            free(header_key);
                                        }
                                        yaml_event_delete(&event);
                                    } else {
                                        yaml_event_delete(&event);
                                    }
                                }
                                if (headers) {
                                    Header *temp = realloc(headers, (count + 1) * sizeof(Header));
                                    if (temp) {
                                        headers = temp;
                                        headers[count].key = NULL;
                                        headers[count].value = NULL;
                                        meta->headers = headers;
                                    } else {
                                        LOG_ERROR("Failed to reallocate headers array");
                                        for (int i = 0; i < count; i++) {
                                            free(headers[i].key);
                                            free(headers[i].value);
                                        }
                                        free(headers);
                                    }
                                }
                            } else {
                                yaml_event_delete(&event);
                            }
                        } else if (strcmp(key, "params") == 0) {
                            if (event.type == YAML_SEQUENCE_START_EVENT) {
                                // Parse params as a sequence of key-value mappings
                                yaml_event_delete(&event);
                                Param *params = NULL;
                                int count = 0;
                                while (1) {
                                    if (!yaml_parser_parse(&parser, &event)) {
                                        break;
                                    }
                                    if (event.type == YAML_SEQUENCE_END_EVENT) {
                                        yaml_event_delete(&event);
                                        break;
                                    }
                                    if (event.type == YAML_MAPPING_START_EVENT) {
                                        yaml_event_delete(&event);
                                        char *param_key = NULL;
                                        char *param_value = NULL;
                                        while (1) {
                                            if (!yaml_parser_parse(&parser, &event)) {
                                                break;
                                            }
                                            if (event.type == YAML_MAPPING_END_EVENT) {
                                                yaml_event_delete(&event);
                                                break;
                                            }
                                            if (event.type == YAML_SCALAR_EVENT) {
                                                char *map_key = strdup((char*)event.data.scalar.value);
                                                if (!map_key) {
                                                    LOG_ERROR("Failed to allocate map_key string");
                                                    yaml_event_delete(&event);
                                                    break;
                                                }
                                                to_lowercase(map_key);
                                                yaml_event_delete(&event);
                                                if (!yaml_parser_parse(&parser, &event)) {
                                                    free(map_key);
                                                    break;
                                                }
                                                if (event.type == YAML_SCALAR_EVENT) {
                                                    if (strcmp(map_key, "key") == 0) {
                                                        param_key = strdup((char*)event.data.scalar.value);
                                                        if (!param_key) LOG_ERROR("Failed to allocate param_key");
                                                    } else if (strcmp(map_key, "value") == 0) {
                                                        param_value = strdup((char*)event.data.scalar.value);
                                                        if (!param_value) LOG_ERROR("Failed to allocate param_value");
                                                    }
                                                    yaml_event_delete(&event);
                                                } else {
                                                    yaml_event_delete(&event);
                                                }
                                                free(map_key);
                                            } else {
                                                yaml_event_delete(&event);
                                            }
                                        }
                                        if (param_key && param_value) {
                                            Param *temp = realloc(params, (count + 1) * sizeof(Param));
                                            if (temp) {
                                                params = temp;
                                                params[count].key = param_key;
                                                params[count].value = param_value;
                                                count++;
                                            } else {
                                                free(param_key);
                                                free(param_value);
                                                LOG_ERROR("Failed to reallocate params array");
                                            }
                                        } else {
                                            free(param_key);
                                            free(param_value);
                                        }
                                    } else {
                                        yaml_event_delete(&event);
                                    }
                                }
                                if (params) {
                                    Param *temp = realloc(params, (count + 1) * sizeof(Param));
                                    if (temp) {
                                        params = temp;
                                        params[count].key = NULL;
                                        params[count].value = NULL;
                                        meta->params = params;
                                    } else {
                                        LOG_ERROR("Failed to reallocate params array");
                                        for (int i = 0; i < count; i++) {
                                            free(params[i].key);
                                            free(params[i].value);
                                        }
                                        free(params);
                                    }
                                }
                            } else if (event.type == YAML_MAPPING_START_EVENT) {
                                // Parse params as direct key-value pairs
                                yaml_event_delete(&event);
                                Param *params = NULL;
                                int count = 0;
                                while (1) {
                                    if (!yaml_parser_parse(&parser, &event)) {
                                        break;
                                    }
                                    if (event.type == YAML_MAPPING_END_EVENT) {
                                        yaml_event_delete(&event);
                                        break;
                                    }
                                    if (event.type == YAML_SCALAR_EVENT) {
                                        char *param_key = strdup((char*)event.data.scalar.value);
                                        if (!param_key) {
                                            LOG_ERROR("Failed to allocate param_key string");
                                            yaml_event_delete(&event);
                                            continue;
                                        }
                                        yaml_event_delete(&event);
                                        if (!yaml_parser_parse(&parser, &event)) {
                                            free(param_key);
                                            break;
                                        }
                                        if (event.type == YAML_SCALAR_EVENT) {
                                            char *param_value = strdup((char*)event.data.scalar.value);
                                            if (!param_value) {
                                                LOG_ERROR("Failed to allocate param_value string");
                                                free(param_key);
                                            } else {
                                                Param *temp = realloc(params, (count + 1) * sizeof(Param));
                                                if (temp) {
                                                    params = temp;
                                                    params[count].key = param_key;
                                                    params[count].value = param_value;
                                                    count++;
                                                } else {
                                                    free(param_key);
                                                    free(param_value);
                                                    LOG_ERROR("Failed to reallocate params array");
                                                }
                                            }
                                        } else {
                                            free(param_key);
                                        }
                                        yaml_event_delete(&event);
                                    } else {
                                        yaml_event_delete(&event);
                                    }
                                }
                                if (params) {
                                    Param *temp = realloc(params, (count + 1) * sizeof(Param));
                                    if (temp) {
                                        params = temp;
                                        params[count].key = NULL;
                                        params[count].value = NULL;
                                        meta->params = params;
                                    } else {
                                        LOG_ERROR("Failed to reallocate params array");
                                        for (int i = 0; i < count; i++) {
                                            free(params[i].key);
                                            free(params[i].value);
                                        }
                                        free(params);
                                    }
                                }
                            } else {
                                yaml_event_delete(&event);
                            }
                        } else if (strcmp(key, "cookies") == 0 && event.type == YAML_SEQUENCE_START_EVENT) {
                            yaml_event_delete(&event);
                            Cookie *cookies = NULL;
                            int count = 0;
                            while (1) {
                                if (!yaml_parser_parse(&parser, &event)) {
                                    break;
                                }
                                if (event.type == YAML_SEQUENCE_END_EVENT) {
                                    yaml_event_delete(&event);
                                    break;
                                }
                                if (event.type == YAML_MAPPING_START_EVENT) {
                                    yaml_event_delete(&event);
                                    char *name = NULL;
                                    char *value = NULL;
                                    char *domain = NULL;
                                    char *path = NULL;
                                    char *expires = NULL;
                                    bool httpOnly = false;
                                    bool secure = false;
                                    while (1) {
                                        if (!yaml_parser_parse(&parser, &event)) {
                                            break;
                                        }
                                        if (event.type == YAML_MAPPING_END_EVENT) {
                                            yaml_event_delete(&event);
                                            break;
                                        }
                                        if (event.type == YAML_SCALAR_EVENT) {
                                            char *map_key = strdup((char*)event.data.scalar.value);
                                            if (!map_key) {
                                                LOG_ERROR("Failed to allocate map_key string");
                                                yaml_event_delete(&event);
                                                break;
                                            }
                                            to_lowercase(map_key);
                                            yaml_event_delete(&event);
                                            if (!yaml_parser_parse(&parser, &event)) {
                                                free(map_key);
                                                break;
                                            }
                                            if (event.type == YAML_SCALAR_EVENT) {
                                                if (strcmp(map_key, "name") == 0) {
                                                    name = strdup((char*)event.data.scalar.value);
                                                    if (!name) LOG_ERROR("Failed to allocate cookie name");
                                                } else if (strcmp(map_key, "value") == 0) {
                                                    value = strdup((char*)event.data.scalar.value);
                                                    if (!value) LOG_ERROR("Failed to allocate cookie value");
                                                } else if (strcmp(map_key, "domain") == 0) {
                                                    domain = strdup((char*)event.data.scalar.value);
                                                    if (!domain) LOG_ERROR("Failed to allocate cookie domain");
                                                } else if (strcmp(map_key, "path") == 0) {
                                                    path = strdup((char*)event.data.scalar.value);
                                                    if (!path) LOG_ERROR("Failed to allocate cookie path");
                                                } else if (strcmp(map_key, "expires") == 0) {
                                                    expires = strdup((char*)event.data.scalar.value);
                                                    if (!expires) LOG_ERROR("Failed to allocate cookie expires");
                                                } else if (strcmp(map_key, "httponly") == 0) {
                                                    httpOnly = strcmp((char*)event.data.scalar.value, "true") == 0;
                                                } else if (strcmp(map_key, "secure") == 0) {
                                                    secure = strcmp((char*)event.data.scalar.value, "true") == 0;
                                                }
                                                yaml_event_delete(&event);
                                            } else {
                                                yaml_event_delete(&event);
                                            }
                                            free(map_key);
                                        } else {
                                            yaml_event_delete(&event);
                                        }
                                    }
                                    if (name && value) {
                                        Cookie *temp = realloc(cookies, (count + 1) * sizeof(Cookie));
                                        if (temp) {
                                            cookies = temp;
                                            cookies[count].name = name;
                                            cookies[count].value = value;
                                            cookies[count].domain = domain ? domain : strdup("");
                                            cookies[count].path = path ? path : strdup("/");
                                            cookies[count].expires = expires ? expires : strdup("");
                                            cookies[count].httpOnly = httpOnly;
                                            cookies[count].secure = secure;
                                            count++;
                                        } else {
                                            free(name);
                                            free(value);
                                            free(domain);
                                            free(path);
                                            free(expires);
                                            LOG_ERROR("Failed to reallocate cookies array");
                                        }
                                    } else {
                                        free(name);
                                        free(value);
                                        free(domain);
                                        free(path);
                                        free(expires);
                                    }
                                } else {
                                    yaml_event_delete(&event);
                                }
                            }
                            if (cookies) {
                                Cookie *temp = realloc(cookies, (count + 1) * sizeof(Cookie));
                                if (temp) {
                                    cookies = temp;
                                    cookies[count].name = NULL;
                                    cookies[count].value = NULL;
                                    cookies[count].domain = NULL;
                                    cookies[count].path = NULL;
                                    cookies[count].expires = NULL;
                                    cookies[count].httpOnly = false;
                                    cookies[count].secure = false;
                                    meta->cookies = cookies;
                                } else {
                                    LOG_ERROR("Failed to reallocate cookies array");
                                    for (int i = 0; i < count; i++) {
                                        free(cookies[i].name);
                                        free(cookies[i].value);
                                        free(cookies[i].domain);
                                        free(cookies[i].path);
                                        free(cookies[i].expires);
                                    }
                                    free(cookies);
                                }
                            }
                        } else {
                            yaml_event_delete(&event);
                        }
                        free(key);
                    } else {
                        yaml_event_delete(&event);
                    }
                }
                done = 1; // Assume single document
                break;
            case YAML_STREAM_END_EVENT:
                done = 1;
                break;
            default:
                break;
        }
        if (!done) {
            yaml_event_delete(&event);
        }
    }

    yaml_parser_delete(&parser);
    return !done;
}

// Convert CURL_METHOD enum to string
const char *method_toString(enum CURL_METHOD method) {
    switch (method) {
        case GET: return "GET";
        case POST: return "POST";
        case PUT: return "PUT";
        case UPDATE: return "UPDATE";
        case _DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

// Print METADATA contents for debugging
void print_metadata(METADATA *metadata) {
    if (!metadata) {
        printf("Metadata is NULL\n");
        return;
    }

    printf("Method: %s\n", method_toString(metadata->method));
    printf("Host: %s\n", metadata->host ? metadata->host : "(null)");
    printf("Path: %s\n", metadata->path ? metadata->path : "(null)");
    printf("URL: %s\n", metadata->url ? metadata->url : "(null)");
    printf("Timeout: %ld\n", metadata->timeout);
    printf("Secure: %s\n", metadata->secure ? "true" : "false");

    printf("Headers:\n");
    if (metadata->headers) {
        for (Header *h = metadata->headers; h->key != NULL; h++) {
            printf("  %s: %s\n", h->key, h->value);
        }
    } else {
        printf("  (none)\n");
    }

    printf("Params:\n");
    if (metadata->params) {
        for (Param *p = metadata->params; p->key != NULL; p++) {
            printf("  %s: %s\n", p->key, p->value);
        }
    } else {
        printf("  (none)\n");
    }

    printf("Cookies:\n");
    if (metadata->cookies) {
        for (Cookie *c = metadata->cookies; c->name != NULL; c++) {
            printf("  name=%s; value=%s; domain=%s; path=%s\n",
                   c->name, c->value, c->domain, c->path);
        }
    } else {
        printf("  (none)\n");
    }
}