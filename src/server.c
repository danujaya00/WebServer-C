#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main()
{

    int server_fd, new_socket;  // discriptors for server file descriptor( listens for incoming connections) and new socket(new socket for the connection)
    struct sockaddr_in address; // holds the address information for the socket
                                // address.sin_family   -> address family Eg: AF_INET is the address family for IPv4.
                                // address.sin_addr.s_addr  -> IP address of the host  Eg: INADDR_ANY listens to all availiable interfaces
                                // address.sin_port   -> Port number on which the server will listen for incoming connections 
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Assign IP, PORT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); // PORT is for 8080

    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for client connections
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);

    // Accept a connection and creates a new connected socket
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("Connection accepted\n");

    // Close the socket
    close(new_socket);
    close(server_fd);

    return 0;
}