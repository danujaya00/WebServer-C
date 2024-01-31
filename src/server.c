#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define PORT 8080
#define BUFFER_SIZE 1024

const char *get_content_type(const char *path)
{

    if (strstr(path, ".html"))
        return "text/html";
    else if (strstr(path, ".css"))
        return "text/css";
    else if (strstr(path, ".js"))
        return "application/javascript";
    else if (strstr(path, ".png"))
        return "image/png";
    else if (strstr(path, ".jpg"))
        return "image/jpeg";
    else if (strstr(path, ".gif"))
        return "image/gif";
    else if (strstr(path, ".ico"))
        return "image/x-icon";
    else if (strstr(path, ".php"))
        return "text/html";
    else
        return "text/plain";
}

void serve_client(int socket)
{
    char buffer[BUFFER_SIZE] = {0};
    char requestType[4];
    char filePath[1024];

    // Read client's request
    ssize_t bytesRead = read(socket, buffer, BUFFER_SIZE);
    if (bytesRead < 0)
    {
        perror("read");
        return;
    }

    if (sscanf(buffer, "%s %s", requestType, filePath) < 2)
    {
        perror("Invalid request");
        return;
    }

    // Default file path if root is requested
    if (strcmp(filePath, "/") == 0)
    {
        strcpy(filePath, "/index.html");
    }

    // Open the file
    char fullPath[1024] = "./../public";
    strcat(fullPath, filePath);
    int file = open(fullPath, O_RDONLY);

    // Check if file exists
    if (file < 0)
    {
        char *notFound = "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<html><body><h1>404 Not Found</h1></body></html>";
        write(socket, notFound, strlen(notFound));
        printf("File not found: %s\n", fullPath);
    }
    else
    {
        struct stat file_stat;
        fstat(file, &file_stat);

        char header[BUFFER_SIZE];
        sprintf(header, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %ld\n\n", get_content_type(fullPath), file_stat.st_size);
        write(socket, header, strlen(header));

        // Send file using zero-copy mechanism
        sendfile(socket, file, NULL, file_stat.st_size);

        close(file);
    }
}

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
    address.sin_port = htons(PORT);

    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for client connections
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);

    // Accept a connection and creates a new connected socket
    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            continue;
        }
        printf("Connection accepted\n");

        serve_client(new_socket);

        printf("page served\n");

        close(new_socket);

        printf("Connection closed\n");
    }
    return 0;
}