#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
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
	accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("Client connected\n");

    // write(server_fd, "Hello, World!\n", 14);
    // char buffer[1024];
    // read(server_fd, buffer, 1024);
    // printf("Client sent: %s\n", buffer);

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