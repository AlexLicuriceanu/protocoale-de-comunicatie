#ifndef _REQUESTS_
#define _REQUESTS_

#include "./json.hpp"

using namespace std;
using json_t = nlohmann::json;

// Computes and returns a DELETE request.
char *compute_delete_request(char *host, char *url, const string& token);

// Computes and returns a GET request string (query_params
// and cookies can be set to NULL if not needed)
char *compute_get_request(char *host, char *url, char *query_params,
							vector<string> cookies, int cookies_count, const string& token);

// Computes and returns a POST request string (cookies can be NULL if not needed)
char *compute_post_request(char *host, char *url, char* content_type, json_t *json, const string& token);

#endif
