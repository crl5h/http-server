#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFF_SIZE 1024

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
	int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("Client connected\n");

    char buffer[BUFF_SIZE];
    int bytes_read = read(client_fd, buffer, BUFF_SIZE);
	if(bytes_read == -1){
		printf("Error reading from client: %s\n", strerror(errno));
		close(client_fd);
		return 1;
	}
	// printf("Client sent: %s\n-- %d bytes\n", buffer, bytes_read);
	// write(client_fd, "HTTP/1.1 200 OK\r\n\r\n", 19);
    
	int i = 4;
	while(i < BUFF_SIZE && buffer[i] != ' '){
		i++;
	}

	int buff_length = i - 5;
	// printf("Buffer length: %d\n", buff_length);
	if(buff_length == 0){
		write(client_fd, "HTTP/1.1 200 OK\r\n\r\n", 19);
	}
	else{
		write(client_fd, "HTTP/1.1 404 Not Found\r\n\r\n", 25);
	}
	
	// close the client and server sockets
	close(client_fd);
	if (server_fd != -1) {
		close(server_fd);
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