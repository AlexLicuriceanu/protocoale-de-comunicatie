#include <iostream>
#include <string>
#include <sys/socket.h>
#include <vector>

#include "./helpers.h"
#include "./requests.h"

// Define some command names.
#define REGISTER_COMMAND "register"
#define LOGIN_COMMAND "login"
#define ENTER_LIBRARY_COMMAND "enter_library"
#define GET_BOOKS_COMMAND "get_books"
#define GET_BOOK_COMMAND "get_book"
#define ADD_BOOK_COMMAND "add_book"
#define DELETE_BOOK_COMMAND "delete_book"
#define LOGOUT_COMMAND "logout"
#define EXIT_COMMAND "exit"

// Define some URL paths.
#define CONTENT_TYPE "application/json"
#define REGISTER_URL "/api/v1/tema/auth/register"
#define LOGIN_URL "/api/v1/tema/auth/login"
#define ACCESS_URL "/api/v1/tema/library/access"
#define BOOKS_URL "/api/v1/tema/library/books"
#define LOGOUT_URL "/api/v1/tema/auth/logout"

// Set the server IP and port.
char server_ip[] = "34.254.242.81";
int server_port = 8080;

using namespace std;
using json_t = nlohmann::json;

/**
 * @brief Converts a string to lowercase.
 *
 * This function takes a string as input and converts all its characters to lowercase using the
 * standard library function tolower(). It modifies the original string and returns it as the
 * result.
 *
 * @param str The string to be converted to lowercase.
 * @return The input string with all characters converted to lowercase.
 */
string lowercase(string str) {

   // Iterate all characters, transform all to lowercase.
   for (auto &c : str) {
       c = (char) tolower(c);
   }
   return str;
}

/**
 * @brief Extracts a cookie from an HTTP response string.
 *
 * This function takes a string containing an HTTP response as input and searches for a "Set-Cookie"
 * header in the response. If found, it extracts the cookie value from the header and returns it as a
 * string. If no "Set-Cookie" header is found, an empty string is returned.
 *
 * @param response The HTTP response string to search for a cookie.
 * @return A string containing the value of the "Set-Cookie" header in the input string, or an empty
 *         string if the header is not found.
 */
string get_cookie(char *response) {
   char *cookie = strstr(response, "Set-Cookie: ");
   char delim[] = " ;";

   if (cookie != nullptr) {
       // Split at "Set-Cookie: ".
       strtok(cookie, delim);
       // Return the cookie, which is at the next split.
       return string(strtok(nullptr, delim));
   }

   return "";
}

/**
 * @brief Extracts a JWT token from an HTTP response string.
 *
 * This function takes a string containing an HTTP response as input and searches for a "token"
 * field in the response. If found, it extracts the JWT token value from the field and returns it as a
 * string. If no "token" field is found, an empty string is returned.
 *
 * @param response The HTTP response string to search for a JWT token.
 * @return A string containing the value of the "token" field in the input string, or an empty
 *         string if the field is not found.
 */
string get_jwt_token(char *response) {
   char *token = strstr(response, "token");
   char delim[] = ":\"";

   if (token != nullptr) {
       // Split at ""token":".
       strtok(token, delim);
       // Return the jwt token.
       return string(strtok(nullptr, delim));
   }

   return "";
}

/**
 * Returns true if the input string does not contain any character from the forbidden string.
 * @param str The input string to check.
 * @param forbidden The string containing characters that are not allowed in the input string.
 * @return True if the input string does not contain any character from the forbidden string, false otherwise.
 */
bool check_chars(const string& str, char forbidden[]) {
   for (auto c : str) {
       if (strchr(forbidden, c)) {
           return false;
       }
   }
   return true;
}

/**
 * Returns true if the input string contains only digits.
 * @param str The input string to check.
 * @return True if the input string contains only digits, false otherwise.
 */
bool check_number(const string& str) {
   for (auto c : str) {
       if (!isdigit(c)) {
           return false;
       }
   }
   return true;
}

/**
 * Main function.
 * @return
 */
int main() {

   char *message;
   char *response;
   int sockfd;

   string session_cookie;
   string jwt_token;

   // Start the infinite loop.
   while (true) {

       // Get the command from the user.
       string command;
       getline(cin, command);
       // Convert the command to lowercase.
       command = lowercase(command);

       // Handle register command.
       if (command == REGISTER_COMMAND) {

           string username, password;

           // Output the register prompt and get the user's input for username and password.
           cout << "username=";
           getline(cin, username);
           cout << "password=";
           getline(cin, password);

           // Validate username and password inputs.
           char forbidden[] = " ";
           if (!check_chars(username, forbidden) || !check_chars(password, forbidden)) {
               continue;
           }

           // Open the connection.
           sockfd = open_connection(server_ip, server_port, AF_INET, SOCK_STREAM, 0);

           // Create the payload.
           json_t json = {
                   {"username", username},
                   {"password", password}
           };

           // Set the URL and content type.
           char url[] = REGISTER_URL;
           char content_type[] = CONTENT_TYPE;

           // Create the message.
           message = compute_post_request(server_ip, url, content_type, &json, "");

           // Send the message to the server.
           send_to_server(sockfd, message);
           // Receive the response from the server.
           response = receive_from_server(sockfd);

           // Close the connection.
           close_connection(sockfd);
           // Print the response.
           cout << response << endl;

           continue;
       }

       // Handle login command.
       if (command == LOGIN_COMMAND) {

           string username, password;

           // Output the login prompt and get the user's input for username and password.
           cout << "username=";
           getline(cin, username);
           cout << "password=";
           getline(cin, password);

           // Validate username and password inputs.
           char forbidden[] = " ";
           if (!check_chars(username, forbidden) || !check_chars(password, forbidden)) {
               continue;
           }

           // Open the connection.
           sockfd = open_connection(server_ip, server_port, AF_INET, SOCK_STREAM, 0);

           // Create the payload.
           json_t json = {
                   {"username", username},
                   {"password", password}
           };

           // Set the URL and content type.
           char url[] = LOGIN_URL;
           char content_type[] = CONTENT_TYPE;

           // Create the message.
           message = compute_post_request(server_ip, url, content_type, &json, "");

           // Send the message to the server.
           send_to_server(sockfd, message);
           // Receive the response from the server.
           response = receive_from_server(sockfd);

           // Close the connection.
           close_connection(sockfd);
           // Print the response.
           cout << response << endl;

           // Get the cookie or "" in case of error.
           session_cookie = get_cookie(response);

           continue;
       }

       // Handle enter_library command.
       if (command == ENTER_LIBRARY_COMMAND) {

           // Open the connection.
           sockfd = open_connection(server_ip, server_port, AF_INET, SOCK_STREAM, 0);

           // Set the URL.
           char url[] = ACCESS_URL;

           // Create the message.
           vector<string> cookies;
           cookies.push_back(session_cookie);
           message = compute_get_request(server_ip, url, nullptr, cookies, cookies.size(), "");

           // Send the message to the server.
           send_to_server(sockfd, message);
           // Receive the response from the server.
           response = receive_from_server(sockfd);

           // Close the connection.
           close_connection(sockfd);
           // Print the response.
           cout << response << endl;

           // Get the token or "" in case of error.
           jwt_token = get_jwt_token(response);

           continue;
       }

       // Handle get_books command.
       if (command == GET_BOOKS_COMMAND) {

           // Open the connection.
           sockfd = open_connection(server_ip, server_port, AF_INET, SOCK_STREAM, 0);

           // Set the URL.
           char url[] = BOOKS_URL;

           // Create the message.
           vector<string> cookies;
           message = compute_get_request(server_ip, url, nullptr, cookies, 0, jwt_token);

           // Send the message to the server.
           send_to_server(sockfd, message);
           // Receive the response from the server.
           response = receive_from_server(sockfd);

           // Close the connection.
           close_connection(sockfd);
           // Print the response.
           cout << response << endl;

           continue;
       }

       // Handle get_book command.
       if (command == GET_BOOK_COMMAND) {

           // Output the ID prompt and get the user's input.
           string id;
           cout << "id=";
           getline(cin, id);

           // Check the user's input.
           if (!check_number(id)) {
               continue;
           }

           // Open the connection.
           sockfd = open_connection(server_ip, server_port, AF_INET, SOCK_STREAM, 0);

           // Set the URL. Length is URL length + "/" + id + "\0".
           auto url_len = strlen(BOOKS_URL) + id.length() + 2;

           // Allocate memory, copy the data.
           char *url = new char[url_len];
           memset(url, 0, sizeof(char) * url_len);
           sprintf(url, "%s/%s", BOOKS_URL, id.c_str());

           // Create the message.
           vector<string> cookies;
           message = compute_get_request(server_ip, url, nullptr, cookies, 0, jwt_token);

           // Send the message to the server.
           send_to_server(sockfd, message);
           // Receive the response from the server.
           response = receive_from_server(sockfd);

           // Close the connection.
           close_connection(sockfd);
           // Print the response.
           cout << response << endl;

           continue;
       }

       // Handle add_book command.
       if (command == ADD_BOOK_COMMAND) {

           // Output the new book prompt and get the user's input.
           string title, author, genre, publisher, page_count;

           cout << "title=";
           getline(cin, title);
           cout << "author=";
           getline(cin, author);
           cout << "genre=";
           getline(cin, genre);
           cout << "publisher=";
           getline(cin, publisher);
           cout << "page_count=";
           getline(cin, page_count);

           // Validate inputs.
           if (!check_number(page_count)) {
               continue;
           }

           // Open the connection.
           sockfd = open_connection(server_ip, server_port, AF_INET, SOCK_STREAM, 0);

           // Create the payload.
           json_t json = {
                   {"title", title},
                   {"author", author},
                   {"genre", genre},
                   {"page_count", stoi(page_count)},
                   {"publisher", publisher}
           };

           // Set the URL and content type.
           char url[] = BOOKS_URL;
           char content_type[] = CONTENT_TYPE;

           // Create the message.
           message = compute_post_request(server_ip, url, content_type, &json, jwt_token);

           // Send the message to the server.
           send_to_server(sockfd, message);
           // Receive the response from the server.
           response = receive_from_server(sockfd);

           // Close the connection.
           close_connection(sockfd);
           // Print the response.
           cout << response << endl;

           continue;
       }

       // Handle delete_book command.
       if (command == DELETE_BOOK_COMMAND) {

           // Output the ID prompt and get the user's input.
           string id;
           cout << "id=";
           getline(cin, id);

           // Check the user's input.
           if (!check_number(id)) {
               continue;
           }

           // Open the connection.
           sockfd = open_connection(server_ip, server_port, AF_INET, SOCK_STREAM, 0);

           // Set the URL. Length is URL length + "/" + id + "\0".
           auto url_len = strlen(BOOKS_URL) + id.length() + 2;

           // Allocate memory, copy the data.
           char *url = new char[url_len];
           memset(url, 0, sizeof(char) * url_len);
           sprintf(url, "%s/%s", BOOKS_URL, id.c_str());

           // Create the message.
           message = compute_delete_request(server_ip, url, jwt_token);

           // Send the message to the server.
           send_to_server(sockfd, message);
           // Receive the response from the server.
           response = receive_from_server(sockfd);

           // Close the connection.
           close_connection(sockfd);
           // Print the response.
           cout << response << endl;

           continue;
       }

       // Handle logout command.
       if (command == LOGOUT_COMMAND) {

           // Set the URL.
           char url[] = LOGOUT_URL;

           // Create the message.
           vector<string> cookies;
           cookies.push_back(session_cookie);
           message = compute_get_request(server_ip, url, nullptr, cookies, cookies.size(), "");

           // Open the connection.
           sockfd = open_connection(server_ip, server_port, AF_INET, SOCK_STREAM, 0);

           // Send the message to the server.
           send_to_server(sockfd, message);
           // Receive the response from the server.
           response = receive_from_server(sockfd);

           // Close the connection.
           close_connection(sockfd);
           // Print the response.
           cout << response << endl;

           // Delete session_cookie and jwt_token.
           session_cookie = "";
           jwt_token = "";
       }

       // Handle exit command.
       if (command == EXIT_COMMAND) {
           break;
       }
   }

   return 0;
}
