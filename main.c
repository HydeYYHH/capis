#include "read_yaml.h"
#include "log.h"
#include "easy_curl.h"
#include "multi_curl.h"
#include "utils.h"
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>

/* 
gcc main.c easy_curl.c multi_curl.c log.c read_yaml.c utils.c -I. -I./curl/include -I.\libyaml\include -L./curl/lib -lcurl -lyaml
*/
int main(int argc, char *argv[]) {
    LOG_INFO("CAPIS RUNNING");

    int verbose = 0;
    StrLList filepaths = init_strllist();

    curl_global_init(CURL_GLOBAL_ALL);

    // Parse command-line arguments
    for (int a = 1; a < argc; a++) {
        char *arg = argv[a];
        if (arg[0] != '-') {
            ap_strllist(filepaths, arg);
        } else if (strcmp(arg, "--verbose") == 0 || strcmp(arg, "-v") == 0) {
            verbose = 1;
        }
    }

    // Process each YAML file
    Node *cur = filepaths->next;
    while (cur) {
        METADATA *md = init_metadata();
        if (!md) {
            LOG_ERROR("Failed to initialize metadata for %s", cur->val);
            continue;
        }

        FILE *fp = fopen(cur->val, "r");
        if (!fp) {
            LOG_ERROR("Failed to open %s", cur->val);
            free_metadata(md);
            cur = cur->next;
            continue;
        }

        LOG_INFO("Processing METADATA: %s", cur->val);
        if (read_yaml(fp, md)) goto SKIP_REQ;
        
        if (verbose) print_metadata(md);

        Response resp = {0}; // Initialize response
        if (do_easy_curl(md, &resp, verbose) == 0) {
            LOG_INFO("Request completed for %s", cur->val);
        } else {
            LOG_ERROR("Request failed for %s", cur->val);
        }

        free_response(&resp);

    SKIP_REQ:
        fclose(fp);
        free_metadata(md);

        cur = cur->next;
    }

    free_strllist(filepaths);
    curl_global_cleanup();
    return 0;
}