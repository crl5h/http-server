#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFF_SIZE 1024
#define HTTP_OK "HTTP/1.1 200 OK\r\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 Not Found\r\n\r\n"
#define CONTENT_TYPE_TEXT "Content-Type: text/plain\r\n"
#define CONTENT_LENGTH "Content-Length:"
#define USER_AGENT_LEN 12
struct Request {
    char http_method[10];
    char path[100];
    char http_protocol[10];
    char user_agent[100];
    char host[100];
    char accept[100];
} request;

struct Reponse {
    char status_code[10];
    char content_type[100];
    char content_length[10];
    char content[50];
} response;

// send_response(client_fd, HTTP_OK, "text/plain", content);
void sendResponse(int client_fd, const char* status, const char* content, int content_length) {
    char response[1024];
    sprintf(response, "%sContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", status, content_length, content);
    // printf("\nResponse: %s\n", response);
    send(client_fd, response, strlen(response), 0);
}

void parseHeader(char* buffer) {
    char* token = strtok(buffer, "\r\n");  // tokenize the buffer
    for (int i = 0; i < 5; i++) {
        // printf("-%s\n", token);
        if (token == NULL) {
            break;
        }
        if (strncmp(token, "User-Agent", 10) == 0) {
            strcat(request.user_agent, token + 12);
        } else if (strncmp(token, "Host", 4) == 0) {
            strcat(request.host, token + 6);
            // printf("->Host is: %s\n", request.host);
        } else if (strncmp(token, "Accept", 6) == 0) {
            strcat(request.accept, token + 8);
            // printf("->Accept is: %s\n", request.accept);
        }
        token = strtok(NULL, "\r\n");
    }
}

int main() {
    // Disable output buffering
    setbuf(stdout, NULL);

    // socket file descriptor
    int server_fd, client_addr_len;

    // client socket address
    struct sockaddr_in client_addr;

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(4221),
        .sin_addr = {htonl(INADDR_ANY)},
    };

    // socket of type ipv4, tcp, default protocol
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    // reuseport allows multiple sockets to bind to the same address and port
    // it avoids the "Address already in use" error
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        printf("SO_REUSEPORT failed: %s \n", strerror(errno));
        return 1;
    }

    // bind the server socket to the address and port
    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    // listen for incoming connections from clients
    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        printf("Listen failed: %s \n", strerror(errno));
        return 1;
    }

    printf("Waiting for a client to connect...\n");
    client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd == -1) {
        printf("Accept failed: %s \n", strerror(errno));
    }
    printf("Client connected\n");

    char buffer[BUFF_SIZE];
    int bytes_read = read(client_fd, buffer, BUFF_SIZE);
    if (bytes_read == -1) {
        printf("Error reading from client: %s\n", strerror(errno));
        close(client_fd);
        return 1;
    }

    // printf("\nbuffer:\n%s\n", buffer);
    // parse method,path,protocol buffer and store in request struct
    sscanf(buffer, "%s%s%s", request.http_method, request.path, request.http_protocol);
    // parses useragent, host, accept
    parseHeader(buffer);

    // printf("->Method: %s\n", request.http_method);
    // printf("->Path: %s\n", request.path);
    // printf("->Protocol: %s\n", request.http_protocol);
    // printf("->Host is: %s\n", request.host);
    // printf("->User-Agent is: %s\n", request.user_agent);
    // printf("->Accept is: %s\n", request.accept);

    // if echo in url
    if (strncmp("/echo/", request.path, 6) == 0) {
        printf("Sending 200...\n");
        char resp[1024];
        char content[100];
        for (int i = 0; i < strlen(request.path); i++) {
            content[i] = request.path[i + 6];
        }
        sendResponse(client_fd, HTTP_OK, content, strlen(content));
    } else if (strcmp("/user-agent", request.path) == 0) {
        printf("Sending 200...\n");
        sendResponse(client_fd, HTTP_OK, request.user_agent, strlen(request.user_agent));
    }
    // if empty path
    else if (strcmp("/", request.path) == 0) {
        printf("Sending 200...\n");
        // char *resp = "HTTP/1.1 200 OK\r\n\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";
        sendResponse(client_fd, HTTP_OK, "", 0);
    }
    // not (empty || echo)
    else {
        printf("Sending 404...\n");
        sendResponse(client_fd, HTTP_NOT_FOUND, "", 0);
    }

    // close the client and server sockets
    close(client_fd);
    close(server_fd);

    return 0;
}

/*

basic architecture of tcp server:
        1. create a socket
        2. bind the socket to an address and port
        3. listen for incoming connections
        4. accept the connection
        5. send and receive data

client:
        1. create a socket
        2. connect to the server
        3. send and receive data
*/