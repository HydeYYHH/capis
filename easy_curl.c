#include "easy_curl.h"
#include "log.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// Implementation of strndup for systems lacking it
char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *new = malloc(len + 1);
    if (!new) return NULL;
    memcpy(new, s, len);
    new[len] = '\0';
    return new;
}

// Free memory allocated for Response struct
void free_response(Response *resp) {
    if (!resp) return;

    if (resp->headers) {
        free(resp->headers);
        resp->headers = NULL;
    }

    if (resp->body) {
        free(resp->body);
        resp->body = NULL;
    }

    if (resp->set_cookies) {
        for (char **c = resp->set_cookies; *c != NULL; c++) {
            free(*c);
        }
        free(resp->set_cookies);
        resp->set_cookies = NULL;
    }

    // No need to free status_code (primitive type)
}

// Callback to handle response body data
static size_t write_body_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    Response *resp = (Response *)userp;

    char *ptr = realloc(resp->body, resp->body_size + realsize + 1);
    if (!ptr) {
        LOG_ERROR("Failed to allocate body buffer");
        return 0;
    }

    resp->body = ptr;
    memcpy(&(resp->body[resp->body_size]), contents, realsize);
    resp->body_size += realsize;
    resp->body[resp->body_size] = '\0';
    return realsize;
}

// Callback to handle response header data
static size_t write_header_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    Response *resp = (Response *)userp;

    char *ptr = realloc(resp->headers, resp->headers_size + realsize + 1);
    if (!ptr) {
        LOG_ERROR("Failed to allocate header buffer");
        return 0;
    }

    resp->headers = ptr;
    memcpy(&(resp->headers[resp->headers_size]), contents, realsize);
    resp->headers_size += realsize;
    resp->headers[resp->headers_size] = '\0';

    const char *set_cookie_prefix = "Set-Cookie:";
    if (strncasecmp((char *)contents, set_cookie_prefix, strlen(set_cookie_prefix)) == 0) {
        char *cookie_line = strndup((char *)contents, realsize);
        if (cookie_line) {
            cookie_line[strcspn(cookie_line, "\r\n")] = '\0';

            size_t count = 0;
            if (resp->set_cookies) {
                while (resp->set_cookies[count] != NULL) count++;
            }

            char **new_list = realloc(resp->set_cookies, sizeof(char *) * (count + 2));
            if (new_list) {
                new_list[count] = strdup(cookie_line);
                new_list[count + 1] = NULL;
                resp->set_cookies = new_list;
            } else {
                LOG_ERROR("Failed to allocate cookie list");
            }

            free(cookie_line);
        }
    }

    return realsize;
}

// Perform HTTP request with metadata and store response
int do_easy_curl(METADATA *md, Response *resp, ...) {
    va_list args;
    va_start(args, resp);
    int verbose = va_arg(args, int);
    va_end(args);

    if (!md || !resp) {
        LOG_ERROR("Invalid metadata or response pointer");
        return -1;
    }

    // Initialize response fields
    resp->headers = NULL;
    resp->body = NULL;
    resp->headers_size = 0;
    resp->body_size = 0;
    resp->set_cookies = NULL;
    resp->status_code = 0;

    CURL *curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("curl_easy_init failed");
        return -1;
    }

    int url_allocated = 0;
    if (!md->url || strlen(md->url) == 0) {
        size_t len = strlen(md->host) + strlen(md->path) + 16;
        md->url = malloc(len);
        if (!md->url) {
            LOG_ERROR("Memory allocation failed for URL");
            curl_easy_cleanup(curl);
            return -1;
        }
        snprintf(md->url, len, "%s://%s%s", md->secure ? "https" : "http", md->host, md->path);
        url_allocated = 1;
    }

    LOG_INFO("Preparing request: %s", md->url);

    curl_easy_setopt(curl, CURLOPT_URL, md->url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, md->timeout);

    if (!md->secure) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        LOG_WARN("SSL verification disabled - security risk");
    }

    struct curl_slist *header_list = NULL;
    bool has_content_type = false;
    if (md->headers) {
        for (char **h = md->headers; *h != NULL; h++) {
            if (strncasecmp(*h, "Content-Type:", 13) == 0) {
                has_content_type = true;
            }
            header_list = curl_slist_append(header_list, *h);
        }
    }

    if (!has_content_type && (md->method == POST || md->method == PUT)) {
        header_list = curl_slist_append(header_list, "Content-Type: application/x-www-form-urlencoded");
    }

    if (header_list) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
    }

    char *cookie_str = NULL;
    if (md->cookies) {
        size_t buf_len = 4096;
        cookie_str = malloc(buf_len);
        if (!cookie_str) {
            LOG_ERROR("Failed to allocate cookie string");
            curl_slist_free_all(header_list);
            curl_easy_cleanup(curl);
            return -1;
        }

        cookie_str[0] = '\0';
        for (Cookie *c = md->cookies; c->name != NULL; c++) {
            size_t entry_len = strlen(c->name) + strlen(c->value) + 3;
            size_t space_left = buf_len - strlen(cookie_str);

            while (space_left < entry_len) {
                buf_len *= 2;
                char *temp = realloc(cookie_str, buf_len);
                if (!temp) {
                    LOG_ERROR("Failed to reallocate cookie string");
                    free(cookie_str);
                    curl_slist_free_all(header_list);
                    curl_easy_cleanup(curl);
                    return -1;
                }
                cookie_str = temp;
                space_left = buf_len - strlen(cookie_str);
            }

            snprintf(cookie_str + strlen(cookie_str), space_left, "%s=%s", c->name, c->value);
            if (c->domain && strlen(c->domain) > 0) {
                snprintf(cookie_str + strlen(cookie_str), space_left, "; Domain=%s", c->domain);
            }
            if (c->path && strlen(c->path) > 0) {
                snprintf(cookie_str + strlen(cookie_str), space_left, "; Path=%s", c->path);
            }
            if (c->expires && strlen(c->expires) > 0) {
                snprintf(cookie_str + strlen(cookie_str), space_left, "; Expires=%s", c->expires);
            }
            if (c->secure) {
                snprintf(cookie_str + strlen(cookie_str), space_left, "; Secure");
            }
            if (c->httpOnly) {
                snprintf(cookie_str + strlen(cookie_str), space_left, "; HttpOnly");
            }
            snprintf(cookie_str + strlen(cookie_str), space_left, "; ");
        }

        size_t cookie_str_len = strlen(cookie_str);
        if (cookie_str_len > 2) {
            cookie_str[cookie_str_len - 2] = '\0';
        }

        curl_easy_setopt(curl, CURLOPT_COOKIE, cookie_str);
    }

    char *param_str = NULL;
    if (md->params && (md->method == POST || md->method == PUT)) {
        size_t param_len = 0;
        for (Param *p = md->params; p->key != NULL; p++) {
            param_len += strlen(p->key) + strlen(p->value) + 2;
        }

        param_str = malloc(param_len + 1);
        if (!param_str) {
            LOG_ERROR("Failed to allocate param string");
            free(cookie_str);
            curl_slist_free_all(header_list);
            curl_easy_cleanup(curl);
            return -1;
        }

        char *p = param_str;
        for (Param *param = md->params; param->key != NULL; param++) {
            p += snprintf(p, param_str + param_len + 1 - p, "%s=%s&", param->key, param->value);
        }
        if (p > param_str && p[-1] == '&') p[-1] = '\0';
        else *p = '\0';

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, param_str);
    }

    switch (md->method) {
        case POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            break;
        case PUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            break;
        case DELETE:
        case _DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        case GET:
        default:
            break;
    }

    if (verbose) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, resp);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        LOG_ERROR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
    } else {
        // Get HTTP status code
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);
        LOG_INFO("Request successful - Status Code: %ld", resp->status_code);
        LOG_INFO("========== RESPONSE HEADERS ==========\n%s", resp->headers ? resp->headers : "(empty)");
        LOG_INFO("========== RESPONSE BODY =============\n%s", resp->body ? resp->body : "(empty)");
        if (resp->set_cookies) {
            for (char **c = resp->set_cookies; *c; c++) {
                LOG_INFO("Set-Cookie: %s", *c);
            }
        }
    }

    free(param_str);
    free(cookie_str);
    if (header_list) curl_slist_free_all(header_list);
    if (url_allocated) {
        free(md->url);
        md->url = NULL;
    }

    curl_easy_cleanup(curl);
    return (res == CURLE_OK) ? 0 : -1;
}