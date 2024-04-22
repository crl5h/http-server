#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFF_SIZE 1024
#define HTTP_OK "HTTP/1.1 200 OK\r\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 Not Found\r\n"
#define CONTENT_TYPE_TEXT "Content-Type: text/plain\r\n"
#define CONTENT_TYPE_APP_OCTET "Content-Type: application/octet-stream\r\n"

#define GREEN "\033[0;32m"
#define RESET "\033[0m"
#define RED "\033[0;31m"

struct Request {
    char *http_method;
    char *path;
    char *http_protocol;
    char *user_agent;
    char *host;
    char *accept;
};

struct targs {
    int client_fd;
    char *dir;
};


void *handle_connection(void *arg);
void parseHeader(char *buffer, struct Request *request);
void set_response(char *buffer, int client_fd, char *dir);
void sendResponse(int client_fd, const char *status, const char *content_type, const char *content, size_t content_length);


int main(int argc, char *argv[]) {
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
            free(client_fd);
            return 1;
        }
        printf("Client connected: %d\n", *client_fd);
        struct targs *targs = malloc(sizeof(struct targs));

        targs->client_fd = *client_fd;
        if (argv[1] != NULL && argv[2] != NULL) {
            targs->dir = argv[2];
        } else {
            targs->dir = "";
            // printf(RED "No directory provided\n" RESET);
        }

        printf(GREEN "dir is --%s\n%d\n" RESET, targs->dir, *client_fd);

        pthread_t tid;
        int tc = pthread_create(&tid, NULL, handle_connection, targs);
        if (tc != 0) {
            printf(RED "Error creating thread: %s\n" RESET, strerror(errno));
            // close(targs->client_fd);
            close(*client_fd);
            // free(client_fd);
            // free(targs);
            free(client_fd);
            free(targs);
            return 1;
        } else {
            printf(RED "detaching thread\n" RESET);
            pthread_detach(tid);
        }
    }

    close(server_fd);
    return 0;
}

void sendResponse(int client_fd, const char *status, const char *content_type, const char *content, size_t content_length) {
    char response[4096];
    if (sprintf(response, "%s%sContent-Length: %zu\r\n\r\n%s\r\n", status, content_type, content_length, content) < 0) {
        printf(RED "Error creating response: %s\n" RESET, strerror(errno));
    }
    printf("Response is--:\n%s\n", response);
    if (send(client_fd, response, strlen(response), 0) == -1) {
        printf(RED "Error sending response: %s\n" RESET, strerror(errno));
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

void set_response(char *buffer, int client_fd, char *dir) {
    struct Request request;

    request.http_method = (char *)malloc(10);
    request.path = (char *)malloc(100);
    request.http_protocol = (char *)malloc(10);

    // printf("Received: %s\n", buffer);
    // write request data to request struct from buffer  
    if (sscanf(buffer, "%s %s %s", request.http_method, request.path, request.http_protocol) != 3) {
        printf("Failed to parse input\n");
        close(client_fd);
    }

    parseHeader(buffer, &request);

    if (strncmp("/echo/", request.path, 6) == 0) {
        printf("Sending 200...\n");
        char content[100];
        size_t content_length = strlen(request.path) - 6;
        strncpy(content, request.path + 6, content_length);
        content[content_length] = '\0';
        sendResponse(client_fd, HTTP_OK, CONTENT_TYPE_TEXT, content, strlen(content));
    } else if (strncmp("/user-agent", request.path, 11) == 0) {
        // green color
        printf("Sending 200...\n");
        sendResponse(client_fd, HTTP_OK, CONTENT_TYPE_TEXT, request.user_agent, strlen(request.user_agent));
    } else if (strlen(request.path) == 1 && strcmp("/", request.path) == 0) {
        printf("Sending 200...\n");
        sendResponse(client_fd, HTTP_OK, CONTENT_TYPE_TEXT, "", 0);
    } else if (strncmp("/files/", request.path, 7) == 0) {
        printf(GREEN "in files\n" RESET);
        if (strncmp(dir, "",1) == 0) {
            printf(RED "No directory provided\n" RESET);
            sendResponse(client_fd, HTTP_NOT_FOUND, CONTENT_TYPE_TEXT, "", 0);
            return;
        }

        char fname[512];
        strcat(fname, dir);
        strcat(fname, request.path + 7);
        printf("\nfilename->%s \n", fname);

        FILE *fd = fopen(fname, "r");
        if (fd == NULL) {
            printf(RED "Error opening file: %s" RESET "\n", strerror(errno));
            sendResponse(client_fd, HTTP_NOT_FOUND, CONTENT_TYPE_TEXT, "", 0);
            memset(buffer, 0, strlen(buffer));
            memset(fname, 0, strlen(fname));
            return;
        }

        // printf(GREEN "Sending 200...(files)\n" RESET);

        long fileLength;
        fseek(fd, 0, SEEK_END);
        fileLength = ftell(fd);
        rewind(fd);

        char *content = (char *)malloc(fileLength);
        size_t byteSize = fread(content, 1, fileLength, fd);

        char buff[BUFF_SIZE];
        sprintf(buff, "%s%sContent-Length: %ld\r\n\r\n", HTTP_OK, CONTENT_TYPE_APP_OCTET, fileLength);
        // strcat(buff, temp);
        strcat(content, "\r\n");
        // printf("Response(127):\n%s\n", buff);
        if (send(client_fd, buff, strlen(buff), 0) == -1) {
            printf(RED "Error sending response: %s\n" RESET, strerror(errno));
        }
        if(send(client_fd, content, fileLength, 0)==-1){
            printf(RED "Error sending response: %s\n" RESET, strerror(errno));
        }
        
        memset(buff, 0, strlen(buff));
        memset(buffer, 0, strlen(buffer));
        memset(content, 0, fileLength);
        memset(fname, 0, strlen(fname));
        free(content);

        fclose(fd);
    } else {
        printf("Sending 404!...\n");
        sendResponse(client_fd, HTTP_NOT_FOUND, CONTENT_TYPE_TEXT, "", 0);
    }
    free(request.http_method);
    free(request.path);
    free(request.http_protocol);
}

void *handle_connection(void *arg) {
    printf(GREEN "SETTING UP RESPONSE\n" RESET);
    struct targs *targs = (struct targs *)arg;

    char buffer[BUFF_SIZE];
    int client_fd = targs->client_fd;
    char *dir = targs->dir;

    free(arg);
    if (recv(client_fd, buffer, sizeof(buffer) - 1, 0) == -1) {
        printf(RED "Error reading from client: %s\n" RESET, strerror(errno));
    }
    set_response(buffer, client_fd, dir);
    // GET /files/<filename>.
    buffer[0] = '\0';
    close(client_fd);
    return NULL;
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