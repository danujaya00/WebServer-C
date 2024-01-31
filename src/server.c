#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>

#define PORT 8081
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
        perror("Error reading from socket");
        return;
    }
    else if (bytesRead == 0)
    {
        printf("Client disconnected\n");
        return;
    }

    printf("\nClient request:\n");
    int linesToPrint = 2;
    int lineCount = 0;
    char *line = strtok(buffer, "\n");
    while (line != NULL && lineCount < linesToPrint)
    {
        printf("%s\n", line);
        line = strtok(NULL, "\n");
        lineCount++;
    }

    // Parse the request line
    if (sscanf(buffer, "%s %s", requestType, filePath) < 2)
    {
        char *badRequest = "HTTP/1.1 400 Bad Request\nContent-Type: text/html\n\n<html><body><h1>400 Bad Request</h1></body></html>";
        write(socket, badRequest, strlen(badRequest));
        printf("Bad request from client\n");
        return;
    }
    if (strcmp(requestType, "GET") == 0)
    {
        // Default file path if root is requested
        if (strcmp(filePath, "/") == 0)
        {
            strcpy(filePath, "/index.html");
        }

        // Open the file
        char fullPath[1024] = "./public";
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
            printf("\nHTTP Response:\n%s", header);

            // Send file using zero-copy mechanism
            ssize_t sentBytes = sendfile(socket, file, NULL, file_stat.st_size);
            if (sentBytes < 0)
            {
                perror("Error sending file");
            }
            else
            {
                printf("Served file: %s\n", fullPath);
            }

            close(file);
        }
    }
    else if (strcmp(requestType, "POST") == 0)
    {

        char *contentLengthHeader = strstr(buffer, "Content-Length:");
        int contentLength = 0;
        if (contentLengthHeader)
        {
            sscanf(contentLengthHeader, "Content-Length: %d", &contentLength);
        }

        // Read the POST data
        char postData[contentLength + 1];
        memset(postData, 0, contentLength + 1);
        if (contentLength > 0)
        {
            if (bytesRead < BUFFER_SIZE)
            {
                // Read the remaining part of the POST data
                read(socket, postData, contentLength);
            }
        }

        // Log POST data
        printf("Received POST data: %s\n", postData);

        char *okResponse = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\nPOST data received.\n";
        write(socket, okResponse, strlen(okResponse));
    }
    else
    {
        char *methodNotAllowed = "HTTP/1.1 405 Method Not Allowed\nContent-Type: text/html\n\n<html><body><h1>Method Not Allowed</h1></body></html>";
        write(socket, methodNotAllowed, strlen(methodNotAllowed));
        printf("Unsupported request method: %s\n", requestType);
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

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
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

        pid_t pid = fork();

        // of pid is 0 then it is child process
        // if pid is > 0 then it is parent process
        // child process will serve the client
        // parent process will close the socket and wait for another client

        if (pid == 0)
        {

            close(server_fd);
            serve_client(new_socket);
            close(new_socket);
            printf("Connection closed\n");
            printf("-----------------------------------\n");
            exit(0);
        }
        else if (pid > 0)
        {

            close(new_socket);
            while (waitpid(-1, NULL, WNOHANG) > 0)
                ; // Clean up zombie processes
        }
        else
        {
            perror("fork");
            close(new_socket);
        }
    }
    return 0;
}