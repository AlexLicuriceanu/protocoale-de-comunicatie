#include <cstdlib>     /* exit, atoi, malloc, free */
#include <string>     /* memcpy, memset */

#include "./helpers.h"
#include "./requests.h"

using namespace std;
using json_t = nlohmann::json;

char *compute_delete_request(char *host, char *url, const string& token)
{

    char *message = (char *) calloc(BUFLEN, sizeof(char));
    char *line = (char *) calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    memset(line, 0, sizeof(char) * LINELEN);
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, sizeof(char) * LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3: add token
    if (!token.empty()) {
        memset(line, 0, sizeof(char) * LINELEN);
        sprintf(line, "Authorization: Bearer %s", token.c_str());
        compute_message(message, line);
    }

    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_get_request(char *host, char *url, char *query_params,
                            vector<string> cookies, int cookies_count, const string& token)
{
    char *message = (char *) calloc(BUFLEN, sizeof(char));
    char *line = (char *) calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != nullptr) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, sizeof(char) * LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (!cookies.empty()) {

        for (int i = 0; i < cookies_count; i++) {
            memset(line, 0, sizeof(char) * LINELEN);
            sprintf(line, "Cookie: %s", cookies[i].c_str());
            compute_message(message, line);
        }
    }

    if (!token.empty()) {
        memset(line, 0, sizeof(char) * LINELEN);
        sprintf(line, "Authorization: Bearer %s", token.c_str());
        compute_message(message, line);
    }

    // Step 4: add final new line
    compute_message(message, "");
    return message;
}


char *compute_post_request(char *host, char *url, char* content_type, json_t *json, const string& token)
{
    char *message = (char *) calloc(BUFLEN, sizeof(char));
    char *line = (char *) calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    memset(line, 0, sizeof(char) * LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    auto payload = json->dump();

    memset(line, 0, sizeof(char) * LINELEN);
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    memset(line, 0, sizeof(char) * LINELEN);
    sprintf(line, "Content-Length: %zu", payload.length());
    compute_message(message, line);

    // Step 4: add token
    if (!token.empty()) {
        memset(line, 0, sizeof(char) * LINELEN);
        sprintf(line, "Authorization: Bearer %s", token.c_str());
        compute_message(message, line);
    }

    // Step 5: add new line at end of header
    compute_message(message, "");

    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);
    strcat(message, payload.c_str());

    free(line);
    return message;
}
