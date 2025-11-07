#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdbool.h>
#include "customer.h"
#include "employee.h"
#include "manager.h"
#include "admin.h"

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

void *handle_client_thread(void *arg);
void handle_client(int client_socket);

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket error");
        return 1;
    }

    struct sockaddr_in addr_server, addr_client;
    socklen_t addr_client_len = sizeof(addr_client);

    // FIXED: zero the struct to avoid uninitialized fields
    memset(&addr_server, 0, sizeof(addr_server));

    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(PORT);
    addr_server.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr *)&addr_server, sizeof(addr_server)) == -1)
    {
        perror("bind error");
        close(server_fd);
        return 1;
    }
    if (listen(server_fd, MAX_CLIENTS) == -1)
    {
        perror("listen error");
        close(server_fd);
        return 1;
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1)
    {
        int *new_client = malloc(sizeof(int));
        if (!new_client)
        {
            perror("malloc");
            break;
        }
        *new_client = accept(server_fd, (struct sockaddr *)&addr_client, &addr_client_len);
        if (*new_client == -1)
        {
            perror("accept error");
            free(new_client);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_thread, (void *)new_client) != 0)
        {
            perror("pthread_create error");
            close(*new_client);
            free(new_client);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}

void *handle_client_thread(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);
    handle_client(client_socket);
    close(client_socket);
    return NULL;
}

void handle_client(int client_socket)
{
    printf("Inside Server Handle_client\n");
    while (1)
    {
        int menu_choice;
        printf("server side before recv 92\n");
        int rec_client = recv(client_socket, &menu_choice, sizeof(menu_choice), 0);
        if (rec_client <= 0)
        {
            printf("Client disconnected\n");
            break;
        }

        printf("server side menu choice %d\n", menu_choice);
        switch (menu_choice)
        {
        case 1:
            handle_customer(client_socket);
            break;
        case 2:
            handle_employee(client_socket);
            break;
        case 3:
            handle_manager(client_socket);
            break;
        case 4:
            handle_admin(client_socket);
            break;
        default:
            break;
        }
    }
}
