// STEFAN MIRUNA ANDREEA 324CA
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <iostream>
#include "helpers.hpp"
#include "requests.hpp"
#include "nlohmann/json.hpp"

using namespace std;
using nlohmann::json;


#define URL_REGISTER "/api/v1/tema/auth/register"
#define URL_LOGIN "/api/v1/tema/auth/login"
#define URL_ENTRY_LIBRARY "/api/v1/tema/library/access"
#define URL_LOGOUT "/api/v1/tema/auth/logout"
#define URL_GET_BOOKS "/api/v1/tema/library/books"
#define URL_ADD_BOOKS "/api/v1/tema/library/books"
#define CONTENT_TYPE "application/json"


/* function that receives a json as parameter, coverts it into a char * and 
adds it on the first position of the char ** body_data that will be received
as parameter by the compute_get_request() function */
char ** transform_json_into_char_double_pointer(json body_data) {
    /* the compute_post_request() function from lab 9 receives a
    char ** body_data as parameter, so we need to convert our json body_data
    into the corresponding format */
    char **body_data_char = (char **) calloc(1, sizeof(char *));
    DIE(body_data_char == NULL, "Couldn't allocate memory for body_data_char\n");

    body_data_char[0] = (char *) calloc(body_data.dump().size() + 1, sizeof(char));
    DIE(body_data_char == NULL, "Couldn't allocate memory for body_data_char[0]\n");

    // the .dump() function from nlohmann/json.hpp converts the json object into a string
    string body_data_string = body_data.dump();

    // now that we have the json converted into a string,
    // we need to convert the string into a char *
    memcpy(body_data_char[0], body_data_string.c_str(), body_data_string.size() + 1);

    return body_data_char;
}

/* this function will be used for the login and register commands. They
both have in common the fact that they offer prompt for introducing an
username and a password and then this function computes them */
char *handle_command_with_credentials(int sockfd, const char *URL) {
    char *username = (char *) calloc(LINELEN, sizeof(char));
    DIE(username == NULL, "Couldn't allocate memory for username\n");

    char *password = (char *) calloc(LINELEN, sizeof(char));
    DIE(password == NULL, "Couldn't allocate memory for password\n");

    string username_string;
    string password_string;

    /* read the username and password from the user as strings
    (read the whole line after prompt) */
    cin.ignore();
    cout << "username=";
    getline(cin, username_string);
    cout << "password=";
    getline(cin, password_string);

    // check if the suername contains any spaces
    if (username_string.find(' ') != string::npos) {
        cout << "ERROR: Username cannot contain spaces\n";
        return NULL;
    }

    // the username is valid (does not contain spaces),
    // so we can copy it into the username char *
    strcpy(username, username_string.c_str());

    // check if the password contains any spaces
    if (password_string.find(' ') != string::npos) {
        cout << "ERROR: Password cannot contain spaces\n";
        return NULL;
    }

    // the password is valid (does not contain spaces),
    // so we can copy it into the username char *
    strcpy(password, password_string.c_str());

    /* We need to prepare the body_data for the http post request, so
    we put the char * username and password into a json object */
    json body_data;
    body_data["username"] = username;
    body_data["password"] = password;

    /* We need to convert the json object into a char ** before
    calling the compute_post_request() function */
    char **body_data_char = transform_json_into_char_double_pointer(body_data);

    char *message = compute_post_request(HOST_IP, URL, CONTENT_TYPE, body_data_char, 1, NULL, 0, NULL);
    
    send_to_server(sockfd, message);
    free(message);
    
    char *response = receive_from_server(sockfd);
    return response;
}

/* a json node has the following format: {"first_member":"second_member"}
this function returns just the second_member without commas from a line 
that has the exact format of a json node */
/* the last line of the response from server is either "ok" or has the
format of a json node (usually stating the error). This function will be
used to extract the second member on the last line*/
char *get_second_member_from_json_node(char *last_line)
{
    // make a copy of the last_line because it will be modified
    char * last_line_copy = (char *) calloc(strlen(last_line) + 1, sizeof(char));
    DIE(last_line_copy == NULL, "Couldn't allocate memory for last_line_copy\n");   
    strcpy(last_line_copy, last_line);

    // only keep the string after the ':' character
    char *p = strtok(last_line_copy, ":");
    p = strtok(NULL, ":");

    char *second_member = strdup(p);

    // remove the first character from the error message (which is '"')
    second_member += 1;

    // remove the last 2 characters from the error message
    // these 2 characters are '"' and '}'
    second_member[strlen(second_member) - 2] = '\0';

    return second_member;
}

void print_error_message(char *last_line)
{
    /* if we have reached this point, it means that the last line of
    the respnse is not the string "ok", but an error message of the
    following format {"error": "error_message"} */
    char *error_message = get_second_member_from_json_node(last_line);

    // add the word "ERROR: " at the beginning of the error message
    cout << "ERROR: "<< error_message << endl;
}

void handle_register_command(int sockfd) {
    char *response = handle_command_with_credentials(sockfd, URL_REGISTER);
    /* if the response is NULL, it means that the user has introduced
    an invalid username or password */
    if (response == NULL) {
        return;
    }

    // get the last row of the response
    char *last_line = strrchr(response, '\n');

    // check if the last line is the string "ok"
    if (strncmp(last_line + 1, "ok", 2) == 0) {
        cout << "SUCCESS: User registered successfully\n";
    } else {
        print_error_message(last_line);
    }

    free(response);

    // close the connection with the server
    close(sockfd);

}

char *get_cookie(char *response)
{
    // get the char * starting with "connect.sid=" until the ';' character from the response
    char *cookie = strstr(response, "connect.sid=");
    
    // keep in cookie only the string until ';'
    cookie = strtok(cookie, ";");

    return cookie;
}

void handle_login_response(char *response, char **cookies, bool &login_flag)
{
    // check if the user is already logged in
    if (login_flag == true) {
        cout << "ERROR: User already logged in\n";
        return;
    }

    // get the last row of the response
    char *last_line = strrchr(response, '\n');

    // check if the last line is the string "ok"
    if (strncmp(last_line + 1, "ok", 2) == 0) {
        // if we are here, it means that the login command was successful
        cout << "SUCCESS: User logged in successfully\n";
        login_flag = true;
        char *cookie = get_cookie(response);

        //modify cookies[0] to contain the cookie
        cookies[0] = strdup(cookie);

        return;
    }

    // if we reached this point, it means that the login command was not successful
    print_error_message(last_line);
}


void handle_login_command(int sockfd, char **cookies, bool &login_flag)
{
    char *response = handle_command_with_credentials(sockfd, URL_LOGIN);
    /* if the response is NULL, it means that the user has introduced
    an invalid username or password */
    if (response == NULL) {
        return;
    }

    handle_login_response(response, cookies, login_flag);

    free(response);

    // close the connection with the server
    close(sockfd);
}

char *handle_entry_library_response(char *response)
{
    // if the response does not contain the string "OK", it means that an error has occured
    if (strstr(response, "OK") == NULL) {
        cout << "ERROR: Could not enter library\n";
        return NULL;
    }

    // if we reached this point, it means that we have entered the library successfully
    cout << "SUCCESS: Entered library\n";

    // the token is situated on the last row of the response
    char *last_line = strrchr(response, '\n');

    /* the last line has a json format, where the second member is the token */
    char *token = get_second_member_from_json_node(last_line);

    return token;
}

void handle_entry_library_command(int sockfd, bool login_flag, char **cookies, char **token)
{
    // check if the user logged in
    if (login_flag == false) {
        cout << "ERROR: User not logged in\n";
        return;
    }

    // check if the user has already entered the library
    if ((*token) != NULL) {
        cout << "ERROR: User has already entered library\n";
        return;
    }

    char *message = compute_get_request(HOST_IP, URL_ENTRY_LIBRARY, NULL, cookies, 1, NULL);
    send_to_server(sockfd, message);
    free(message);

    char *response = receive_from_server(sockfd);
    char *library_token = handle_entry_library_response(response);

    if (library_token != NULL) {
        (*token) = library_token;
    }

    close(sockfd);

}

void handle_logout_response(char *response, bool &login_flag, char **token)
{
    // get the last row of the response
    char *last_line = strrchr(response, '\n');

    /* check if the last line is the string "ok", meaning
    that the user logged out successfully */
    if (strncmp(last_line + 1, "ok", 2) == 0) {
        cout << "SUCCESS: User logged out successfully\n";
        login_flag = false;
        (*token) = NULL;
        return;
    }

    // if we reached this point, it means that the logout command was not successful
    print_error_message(last_line);

}

void handle_logout_command(int sockfd, bool &login_flag, char **cookies, char **token)
{
    // check if the user is logged in
    if (login_flag == false) {
        cout << "ERROR: User not logged in\n";
        return;
    }

    char *message = compute_get_request(HOST_IP, URL_LOGOUT, NULL, cookies, 1, NULL);
    send_to_server(sockfd, message);
    free(message);

    char *response = receive_from_server(sockfd);
    handle_logout_response(response, login_flag, token);

    free(response);
    close(sockfd);
}

void handle_get_books_response(char *response)
{
    // get the last row of the response
    char *last_line = strrchr(response, '\n');

    // check if last_line contains the string "error"
    if (strstr(last_line, "error") != NULL) {
        print_error_message(last_line);
        return;
    }

    // if we reached this point, it means that the response is a json object
    cout << json::parse(last_line).dump(4) << endl;
}

void handle_get_books_command(int sockfd, bool login_flag, char **cookies, char **token)
{
    // check if the user is logged in
    if (login_flag == false) {
        cout << "ERROR: User not logged in\n";
        return;
    }

    // check if the user has entered the library
    if ((*token) == NULL) {
        cout << "ERROR: User has not entered library\n";
        return;
    }

    // make a copy of the token because it will be modified
    char *copy_token = (char *) calloc(strlen((*token)) + 1, sizeof(char));
    strcpy(copy_token, (*token));

    char *message = compute_get_request(HOST_IP, URL_GET_BOOKS, NULL, cookies, 1, copy_token);

    // restore token
    (*token) = copy_token;

    send_to_server(sockfd, message);
    free(message);

    char *response = receive_from_server(sockfd);
    handle_get_books_response(response);

    free(response);
    close(sockfd);
}

/* function that returns true if the given string is
a number and false otherwise */
bool is_number(char *s)
{
    // check if each character of the string is a digit
    for (size_t i = 0; i < strlen(s); i++) {
        if (!isdigit(s[i])) {
            return false;
        }
    }

    return true;
}

void handle_add_book_response(char *response)
{
    // get the last row of the response
    char *last_line = strrchr(response, '\n');

    /* check if the last line is the string "ok",
    meaning that the book was added successfully */
    if (strncmp(last_line + 1, "ok", 2) == 0) {
        cout << "SUCCESS: Book added successfully\n";
        return;
    }

    print_error_message(last_line);
}


void handle_add_book_command(int sockfd, bool login_flag, char **cookies, char **token)
{
    // check if the user is logged in
    if (login_flag == false) {
        cout << "ERROR: User not logged in\n";
        return;
    }

    // check if the user has entered the library
    if ((*token) == NULL) {
        cout << "ERROR: User has not entered library\n";
        return;
    }

    // make a copy of the token because it will be modified and we need to restore it
    char *token_copy = (char *) calloc(strlen((*token)) + 1, sizeof(char));
    strcpy(token_copy, (*token));

    // read the book details from the user as strings and then convert them to char *
    cin.ignore();
    cout << "title=";
    string title;
    getline(cin, title);
    char *title_char = (char *) calloc(title.size() + 1, sizeof(char));
    strcpy(title_char, title.c_str());

    cout << "author=";
    string author;
    getline(cin, author);
    char *author_char = (char *) calloc(author.size() + 1, sizeof(char));
    strcpy(author_char, author.c_str());

    cout << "genre=";
    string genre;
    getline(cin, genre);
    char *genre_char = (char *) calloc(genre.size() + 1, sizeof(char));
    strcpy(genre_char, genre.c_str());

    cout << "publisher=";
    string publisher;
    getline(cin, publisher);
    char *publisher_char = (char *) calloc(publisher.size() + 1, sizeof(char));
    strcpy(publisher_char, publisher.c_str());

    cout << "page_count=";
    string page_count;
    getline(cin, page_count);
    char *page_count_char = (char *) calloc(page_count.size() + 1, sizeof(char));
    strcpy(page_count_char, page_count.c_str());

    // check if the page_count is a number
    if (!is_number(page_count_char)) {
        cout << "ERROR: Page count is not a number\n";
        return;
    }

    // We need to prepare the body_data for the http post request
    // fill in the fields of the json object
    json body_data;
    body_data["title"] = title_char;
    body_data["author"] = author_char;
    body_data["genre"] = genre_char;
    body_data["publisher"] = publisher_char;
    body_data["page_count"] = page_count_char;

    /* We need to convert the json object into a char ** before
    calling the compute_post_request() function */
    char **body_data_char = transform_json_into_char_double_pointer(body_data);

    // restore token
    (*token) = token_copy;

    char *message = compute_post_request(HOST_IP, URL_ADD_BOOKS, CONTENT_TYPE, body_data_char, 1, cookies, 1, (*token));
    send_to_server(sockfd, message);
    free(message);
    char *response = receive_from_server(sockfd);

    handle_add_book_response(response);

    free(response);
    close(sockfd);
}

void handle_get_book_response(char *response)
{
    // get the last row of the response
    char *last_line = strrchr(response, '\n');

    // check if last_line contains the string "error"
    if (strstr(last_line, "error") != NULL) {
        print_error_message(last_line);
        return;
    }

    // if we reached this point, it means that the response is a json object
    cout << json::parse(last_line).dump(4) << endl;
}

/* function that returns the url for the get_book command by
concatenating the book_id to the URL_GET_BOOKS */
char *get_url_book(char *book_id_char)
{
    char *url_get_book = (char *) calloc(strlen(URL_GET_BOOKS) + 1, sizeof(char));
    strcpy(url_get_book, URL_GET_BOOKS);
    url_get_book = strcat(url_get_book, "/");
    url_get_book = strcat(url_get_book, book_id_char);
    return url_get_book;
}

char *read_and_verify_validity_book_id()
{
    // read the book_id from the user as a string and then convert it to char *
    cin.ignore();
    cout << "id=";
    string id;
    getline(cin, id);
    // check if the user has introduced a book_id
    if (id.size() == 0) {
        cout << "ERROR: Book id cannot be empty\n";
        return NULL;
    }
    // convert book_id from string to char *
    char *book_id_char = (char *) calloc(id.size() + 1, sizeof(char));
    strcpy(book_id_char, id.c_str());

    // check if the book_id is a number
    if (!is_number(book_id_char)) {
        cout << "ERROR: Book id is not a number\n";
        return NULL;
    }

    return book_id_char;
}

void handle_get_book_command(int sockfd, bool login_flag, char **cookies, char **token)
{
    // check if the user is logged in
    if (login_flag == false) {
        cout << "ERROR: User not logged in\n";
        return;
    }

    // check if the user has entered the library
    if ((*token) == NULL) {
        cout << "ERROR: User has not entered library\n";
        return;
    }

    // make a copy of the token because it will be modified and we need to restore it
    char *token_copy = (char *) calloc(strlen((*token)) + 1, sizeof(char));
    strcpy(token_copy, (*token));

    char *book_id_char = read_and_verify_validity_book_id();

    char *url_get_book = get_url_book(book_id_char);
    char *message = compute_get_request(HOST_IP, url_get_book, book_id_char, cookies, 1, token_copy);

    // restore token
    (*token) = token_copy;

    send_to_server(sockfd, message);
    free(message);

    char *response = receive_from_server(sockfd);
    handle_get_book_response(response);

    free(response);
    close(sockfd);
    
}

void handle_delete_book_response(char *response)
{
    // get the last row of the response
    char *last_line = strrchr(response, '\n');

    /* check if last_line is the string "ok", meaning
    that the book was deleted successfully */
    if (strncmp(last_line + 1, "ok", 2) == 0) {
        cout << "SUCCESS: Book deleted successfully\n";
        return;
    }

    /* if we have reached this point, it means that the last line of
    the respnse is not the string "ok", but an error message of the
    following format {"error": "error_message"} */
    char *error_message = get_second_member_from_json_node(last_line);

    // if error_message starts with "No book", it means that the given id was not found in the list
    if (strncmp(error_message, "No book", 7) == 0) {
        cout << "ERROR: Book id not found\n";
        return;
    }

    // add the word "ERROR: " at the beginning of the error message
    cout << "ERROR: "<< error_message << endl;

}

void handle_delete_book_command(int sockfd, bool login_flag, char **cookies, char **token)
{
    // check if the user is logged in
    if (login_flag == false) {
        cout << "ERROR: User not logged in\n";
        return;
    }

    // check if the user has entered the library
    if ((*token) == NULL) {
        cout << "ERROR: User has not entered library\n";
        return;
    }

    // make a copy of the token because it will be modified and we need to restore it
    char *token_copy = (char *) calloc(strlen((*token)) + 1, sizeof(char));
    strcpy(token_copy, (*token));

    char *book_id_char = read_and_verify_validity_book_id();
    
    char *url_get_book = get_url_book(book_id_char);
    char *message = compute_delete_request(HOST_IP, url_get_book, cookies, 1, token_copy);

    // restore token
    (*token) = token_copy;

    send_to_server(sockfd, message);
    free(message);

    char *response = receive_from_server(sockfd);

    handle_delete_book_response(response);

    free(response);
    close(sockfd);
}

int main(int argc, char *argv[])
{
    int sockfd;

    // allocate memory for cookies;
    /* we will only need one cookie, but the functions
    from requests.cpp (taken from the 9th lab) require a char **,
    so we will allocate memory for a char ** with only one element */
    char **cookies = (char **)calloc(1, sizeof(char *));
    cookies[0] = (char *)calloc(LINELEN, sizeof(char));
    DIE(cookies == NULL, "Couldn't allocate memory for cookies\n");

    // flag that will be true if an user is logged in and false otherwise
    bool login_flag = false;

    // the token will be different from NULL only if the user has entered the library
    char *token = NULL;

    while(1) {
        sockfd = open_connection(HOST_IP, PORT, AF_INET, SOCK_STREAM, 0);

        char *command = (char *) calloc(LINELEN, sizeof(char));
        DIE(command == NULL, "Couldn't allocate memory for command\n");
        cin >> command;

        if (strcmp(command, "register") == 0) {
            handle_register_command(sockfd);
            continue;
        }

        if (strcmp(command, "login") == 0) {
            handle_login_command(sockfd, cookies, login_flag);
            continue;
        }

        if (strcmp(command, "enter_library") == 0) {
            handle_entry_library_command(sockfd, login_flag, cookies, &token);
            continue;
        }

        if (strcmp(command, "logout") == 0) {
            handle_logout_command(sockfd, login_flag, cookies, &token);
            continue;
        }

        if (strcmp(command, "get_books") == 0) {
            handle_get_books_command(sockfd, login_flag, cookies, &token);
            continue;
        }

        if (strcmp(command, "add_book") == 0) {
            handle_add_book_command(sockfd, login_flag, cookies, &token);
            continue;
        }

        if (strcmp(command, "get_book") == 0) {
            handle_get_book_command(sockfd, login_flag, cookies, &token);
            continue;
        }

        if (strcmp(command, "delete_book") == 0) {
            handle_delete_book_command(sockfd, login_flag, cookies, &token);
            continue;
        }

        if (strcmp(command, "exit") == 0) {
            close(sockfd);
            break;
        }

        cout << "Invalid command\n";
    }

    return 0;
}
