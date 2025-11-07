#ifndef ADMIN_H
#define ADMIN_H

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

bool handle_admin(int client_socket)
{
    char username_buffer[128];
    char password_buffer[128];
    int userid_buffer = -1;

    bool authenticated = false;
    int auth_userid = -1;

    /* Receive username */
    int rec_client = recv(client_socket, username_buffer, sizeof(username_buffer) - 1, 0);
    if (rec_client <= 0)
    {
        if (rec_client == 0)
            fprintf(stderr, "Client closed connection before admin username recv\n");
        else
            perror("recv error in admin username");
        return false;
    }
    username_buffer[rec_client] = '\0';

    /* Receive password */
    rec_client = recv(client_socket, password_buffer, sizeof(password_buffer) - 1, 0);
    if (rec_client <= 0)
    {
        if (rec_client == 0)
            fprintf(stderr, "Client closed connection before admin password recv\n");
        else
            perror("recv error in admin password");
        return false;
    }
    password_buffer[rec_client] = '\0';

    /* Receive user id */
    rec_client = recv(client_socket, &userid_buffer, sizeof(userid_buffer), 0);
    if (rec_client <= 0)
    {
        if (rec_client == 0)
            fprintf(stderr, "Client closed connection before admin userid recv\n");
        else
            perror("recv error in admin userid");
        return false;
    }

    /* Authenticate */
    if (!authenticate_admin(client_socket, userid_buffer, password_buffer))
    {
        send(client_socket, "Invalid credential(s).\n", strlen("Invalid credential(s).\n"), 0);
        return false;
    }

    authenticated = true;
    auth_userid = userid_buffer;
    send(client_socket, "Authentication Successful", strlen("Authentication Successful"), 0);

    /* Admin menu loop */
    while (1)
    {
        int menu_options_recv = 0;
        int client_rcv = recv(client_socket, &menu_options_recv, sizeof(menu_options_recv), 0);
        if (client_rcv <= 0)
        {
            if (authenticated)
                admin_logout(client_socket, auth_userid);
            return false;
        }

        switch (menu_options_recv)
        {
        case 1:
            add_employee(client_socket);
            break;
        case 2:
            edit_customer(client_socket);
            break;
        case 3:
            edit_employee(client_socket);
            break;
        case 4:
        {
            int employee_id;
            int rcv_emp_id = recv(client_socket, &employee_id, sizeof(employee_id), 0);
            if (rcv_emp_id <= 0)
            {
                if (authenticated)
                    admin_logout(client_socket, auth_userid);
                return false;
            }
            manage_user_role(client_socket, employee_id);
            break;
        }
        case 5:
            edit_credentials_admin(client_socket, userid_buffer);
            break;
        case 6:
            admin_logout(client_socket, userid_buffer);
            return true;
        case 7:
            // if (authenticated)
            //     admin_logout(client_socket, auth_userid);
            // send(client_socket, "Exiting..", strlen("Exiting.."), 0);
            // close(client_socket);
            // exit(0);
            send(client_socket, "Exiting...\n", strlen("Exiting...\n"), 0);
            if (authenticated)
                admin_logout(client_socket, auth_userid);
            close(client_socket);
            exit(0);
        default:
            break;
        }
    }

    if (authenticated)
        admin_logout(client_socket, auth_userid);
    return false;
}

#endif
