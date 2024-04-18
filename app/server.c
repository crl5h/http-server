#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
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
    char *http_method;
    char *path;
    char *http_protocol;
    char *user_agent;
    char *host;
    char *accept;
};

void sendResponse(int client_fd, const char *status, const char *content, int content_length) {
    char response[1024];
    if (sprintf(response, "%sContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", status, content_length, content) < 0) {
        printf("Error creating response: %s\n", strerror(errno));
    }
    printf("Response: %s\n", response);
    if (send(client_fd, response, strlen(response), 0) == -1) {
        printf("Error sending response: %s\n", strerror(errno));
    }
}

void parseHeader(char *buffer, struct Request *request) {
    char *token = strtok(buffer, "\r\n");  // tokenize the buffer
    for (int i = 0; i < 5; i++) {
        if (token == NULL) {
            break;
        }
        if (strncmp(token, "User-Agent", 10) == 0) {
            request->user_agent = token + 12;
        } else if (strncmp(token, "Host", 4) == 0) {
            request->host = token + 6;
        } else if (strncmp(token, "Accept", 6) == 0) {
            request->accept = token + 8;
        }
        token = strtok(NULL, "\r\n");
    }
}

void *handle_connection(void *arg) {
    
    // char buffer[BUFF_SIZE];
    // memset(buffer, 0, BUFF_SIZE);
    char buffer[BUFF_SIZE] = {0};

    int client_fd = *(int *)arg;
    free(arg);

    if (recv(client_fd, buffer, sizeof(buffer) - 1, 0) == -1) {
        printf("Error reading from client: %s\n", strerror(errno));
        close(client_fd);
    }

    struct Request request;
    
    request.http_method = (char *)malloc(10);
    request.path = (char *)malloc(100);
    request.http_protocol = (char *)malloc(10);

    // printf("Received: %s\n", buffer);
    if (sscanf(buffer, "%s %s %s", request.http_method, request.path, request.http_protocol) != 3) {
        printf("Failed to parse input\n");
        close(client_fd);
    }
    parseHeader(buffer, &request);

    if (strncmp("/echo/", request.path, 6) == 0) {
        printf("Sending 200...\n");
        char content[100];
        int content_length = strlen(request.path) - 6;
        strncpy(content, request.path + 6, content_length);
        content[content_length] = '\0';
        sendResponse(client_fd, HTTP_OK, content, strlen(content));
    } else if (strcmp("/user-agent", request.path) == 0) {
        printf("Sending 200...\n");
        sendResponse(client_fd, HTTP_OK, request.user_agent, strlen(request.user_agent));
    } else if (strcmp("/", request.path) == 0) {
        printf("Sending 200...\n");
        sendResponse(client_fd, HTTP_OK, "", 0);
    } else {
        printf("Sending 404...\n");
        sendResponse(client_fd, HTTP_NOT_FOUND, "", 0);
    }
    close(client_fd);
}

int main() {
    setbuf(stdout, NULL);

    int server_fd;

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(4221),
        .sin_addr = {htonl(INADDR_ANY)},
    };

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        printf("SO_REUSEPORT failed: %s \n", strerror(errno));
        return 1;
    }

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        printf("Listen failed: %s \n", strerror(errno));
        return 1;
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        printf("Waiting for a client to connect...\n");

        client_addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

        if (*client_fd == -1) {
            printf("Accept failed: %s \n", strerror(errno));
        }
        printf("Client connected\n");

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_connection, client_fd) != 0) {
            printf("Error creating thread: %s\n", strerror(errno));
            close(*client_fd);
        } else {
            pthread_detach(tid);
        }
    }

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