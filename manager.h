#ifndef MANAGER_H
#define MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/socket.h>

#include "commonStruct.h"
#include "transaction.h"
#include "credentials.h"
#include "loan.h"
#include "feedback.h"
#include "transactionHistory.h"
#include "viewBalance.h"
#include "addEdit.h"

bool handle_manager(int client_socket)
{
    char username_buffer[128];
    char password_buffer[128];
    int userid_buffer = -1;
    int menu_options_recv = 0;

    bool authenticated = false;
    int auth_userid = -1;

    /* Receive username */
    int rec_client = recv(client_socket, username_buffer, sizeof(username_buffer) - 1, 0);
    if (rec_client <= 0)
    {
        if (rec_client == 0)
            fprintf(stderr, "Client closed before manager username recv\n");
        else
            perror("recv error manager username");
        return false;
    }
    username_buffer[rec_client] = '\0';

    /* Receive password */
    rec_client = recv(client_socket, password_buffer, sizeof(password_buffer) - 1, 0);
    if (rec_client <= 0)
    {
        if (rec_client == 0)
            fprintf(stderr, "Client closed before manager password recv\n");
        else
            perror("recv error manager password");
        return false;
    }
    password_buffer[rec_client] = '\0';

    /* Receive user id */
    rec_client = recv(client_socket, &userid_buffer, sizeof(userid_buffer), 0);
    if (rec_client <= 0)
    {
        if (rec_client == 0)
            fprintf(stderr, "Client closed before manager userid recv\n");
        else
            perror("recv error manager userid");
        return false;
    }

    /* Authenticate */
    if (!authenticate_manager(client_socket, userid_buffer, password_buffer))
    {
        send(client_socket, "Error Loging in.\n", strlen("Error Loging in.\n"), 0);
        return false;
    }

    authenticated = true;
    auth_userid = userid_buffer;
    send(client_socket, "Authentication Successful", strlen("Authentication Successful"), 0);

    /* Manager menu loop */
    while (1)
    {
        int client_rcv = recv(client_socket, &menu_options_recv, sizeof(menu_options_recv), 0);
        if (client_rcv <= 0)
        {
            if (authenticated)
                manager_logout(client_socket, auth_userid);
            return false;
        }

        switch (menu_options_recv)
        {
        case 1:
        {
            char buff_cuss[1024];
            int r = recv(client_socket, buff_cuss, sizeof(buff_cuss) - 1, 0);
            if (r <= 0)
            {
                if (authenticated)
                    manager_logout(client_socket, auth_userid);
                return false;
            }
            buff_cuss[r] = '\0';
            int customer_id = atoi(buff_cuss);
            deactivate_customer(customer_id);
            break;
        }

        case 2:
            view_applied_loan_applications();
            break;

        case 3:
            assign_loan(client_socket);
            break;

        case 4: // Review Feedback
        {
            int customer_id;
            if (recv(client_socket, &customer_id, sizeof(customer_id), 0) <= 0)
                return false;

            resolve_feedback(client_socket, customer_id); // sends feedback text or "no unresolved" message

            char response;
            if (recv(client_socket, &response, sizeof(response), 0) > 0)
            {
                update_feedback_status(client_socket, customer_id, response);
            }
            break;
        }

        case 5:
            edit_credentials_manager(client_socket, userid_buffer);
            break;

        case 6:
            manager_logout(client_socket, userid_buffer);
            return true;

        case 7:
            send(client_socket, "Exiting...\n", strlen("Exiting...\n"), 0);
            if (authenticated)
                manager_logout(client_socket, auth_userid);
            close(client_socket);
            exit(0);

        default:
            break;
        }
    }

    if (authenticated)
        manager_logout(client_socket, auth_userid);
    return false;
}

#endif // MANAGER_H
