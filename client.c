#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include "transactionHistory.h"
#include "customer.h"
#include "employee.h"
#include "manager.h"
#include "admin.h"

/**
 * @brief Client-side implementation for a network-based banking system.
 * * This program connects to a server and provides a user interface for various
 * banking roles, including Customer, Employee, Manager, and Admin.
 */
int main()
{
    // Configure server connection settings
    char *server_address = "127.0.0.1"; // Localhost for the server
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        perror("Socket creation error");
        exit(1);
    }

    // Set up the server's address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    inet_pton(AF_INET, server_address, &serverAddr.sin_addr);

    // Establish a connection to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Connection to server failed");
        close(clientSocket);
        exit(1);
    }

    // Main application loop to handle user interaction
    while (1)
    {
        // Display role selection menu
        printf("Select your role by entering the corresponding number:\n"
               "1. Customer\n2. Employee\n3. Manager\n4. Admin\n");

        // Variables for user authentication
        int menu_choice;
        char username[1024];
        char password[1024];
        int userid;

        if (scanf("%d", &menu_choice) != 1)
        {
            // Clear invalid input from stdin
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            continue;
        }

        // Send the selected role to the server
        if (send(clientSocket, &menu_choice, sizeof(menu_choice), 0) <= 0)
        {
            perror("Failed to send menu choice");
            break;
        }

        switch (menu_choice)
        {
        case 1: // Customer Functionality
        {
            // Get credentials for authentication
            printf("Username: ");
            if (scanf("%1023s", username) != 1)
            {
                username[0] = '\0';
            }
            // Send username including the null terminator
            if (send(clientSocket, username, strlen(username) + 1, 0) <= 0)
            {
                perror("Failed to send username");
                break;
            }

            printf("Password: ");
            if (scanf("%1023s", password) != 1)
            {
                password[0] = '\0';
            }
            if (send(clientSocket, password, strlen(password) + 1, 0) <= 0)
            {
                perror("Failed to send password");
                break;
            }

            printf("User ID: ");
            if (scanf("%d", &userid) != 1)
            {
                userid = -1; // Invalid ID
            }

            if (send(clientSocket, &userid, sizeof(userid), 0) <= 0)
            {
                perror("Failed to send user ID");
                break;
            }

            // Await authentication response from server
            char auth_buffer[1024];
            memset(auth_buffer, 0, sizeof(auth_buffer));
            int auth_rcv = recv(clientSocket, auth_buffer, sizeof(auth_buffer) - 1, 0);
            if (auth_rcv <= 0)
            {
                if (auth_rcv == 0)
                    fprintf(stderr, "Connection closed by server\n");
                else
                    perror("Failed to receive authentication response");
                break;
            }
            auth_buffer[auth_rcv] = '\0';

            // Check if authentication was successful
            if (strcmp("Authentication Successful", auth_buffer) == 0 || strcmp("admin authenticated", auth_buffer) == 0 || strcmp("authenticated", auth_buffer) == 0)
            {
                printf("Login successful.\n");
                while (1)
                {
                    // Display customer action menu
                    char menu_options[] = "Choose an action (enter the corresponding number):\n"
                                          "1. View Account Balance\n2. Deposit Funds\n3. Withdraw Funds\n"
                                          "4. Transfer Funds\n5. Apply For Loan\n6. Change Password\n"
                                          "7. Provide Feedback\n8. View Transaction History\n"
                                          "9. Logout\n10. Exit Application\n";

                    printf("%s", menu_options);
                    int user_choice;
                    if (scanf("%d", &user_choice) != 1)
                    {
                        int ch;
                        while ((ch = getchar()) != '\n' && ch != EOF) {}
                        continue;
                    }
                    if (send(clientSocket, &user_choice, sizeof(user_choice), 0) <= 0)
                    {
                        perror("Failed to send user choice");
                        break;
                    }

                    switch (user_choice)
                    {
                    case 1: // View Balance
                    {
                        char view_buff[1024];
                        memset(view_buff, 0, sizeof(view_buff));
                        int rec_view = recv(clientSocket, view_buff, sizeof(view_buff) - 1, 0);
                        if (rec_view <= 0)
                        {
                            perror("Failed to receive balance data");
                            break;
                        }
                        view_buff[rec_view] = '\0';
                        printf("%s\n", view_buff);
                        break;
                    }

                    case 2: // Deposit
                    {
                        printf("Enter deposit amount: ");
                        int deposit;
                        if (scanf("%d", &deposit) != 1)
                        {
                            deposit = 0;
                        }
                        if (send(clientSocket, &deposit, sizeof(deposit), 0) <= 0)
                        {
                            perror("Failed to send deposit amount");
                            break;
                        }

                        char deposit_buf[1024];
                        memset(deposit_buf, 0, sizeof(deposit_buf));
                        int rec_deposit = recv(clientSocket, deposit_buf, sizeof(deposit_buf) - 1, 0);
                        if (rec_deposit <= 0)
                        {
                            perror("Failed to receive deposit confirmation");
                            break;
                        }
                        deposit_buf[rec_deposit] = '\0';
                        printf("%s\n", deposit_buf);
                        break;
                    }

                    case 3: // Withdraw
                    {
                        printf("Enter withdrawal amount: ");
                        int withdraw;
                        if (scanf("%d", &withdraw) != 1)
                        {
                            withdraw = 0;
                        }
                        if (send(clientSocket, &withdraw, sizeof(withdraw), 0) <= 0)
                        {
                            perror("Failed to send withdrawal amount");
                            break;
                        }

                        char withdraw_buff[1024];
                        memset(withdraw_buff, 0, sizeof(withdraw_buff));
                        int rec_withdraw = recv(clientSocket, withdraw_buff, sizeof(withdraw_buff) - 1, 0);
                        if (rec_withdraw <= 0)
                        {
                            perror("Failed to receive withdrawal confirmation");
                            break;
                        }
                        withdraw_buff[rec_withdraw] = '\0';
                        printf("%s\n", withdraw_buff);
                        break;
                    }

                    case 4: // Transfer Funds
                    {
                        printf("Enter recipient's user ID: ");
                        int rec_id;
                        if (scanf("%d", &rec_id) != 1)
                        {
                            rec_id = 0;
                        }
                        if (send(clientSocket, &rec_id, sizeof(rec_id), 0) <= 0)
                        {
                            perror("Failed to send recipient ID");
                            break;
                        }

                        printf("Enter transfer amount: ");
                        int tr_amount;
                        if (scanf("%d", &tr_amount) != 1)
                        {
                            tr_amount = 0;
                        }
                        if (send(clientSocket, &tr_amount, sizeof(tr_amount), 0) <= 0)
                        {
                            perror("Failed to send transfer amount");
                            break;
                        }

                        char tr_buffer[1024];
                        memset(tr_buffer, 0, sizeof(tr_buffer));
                        int rec_tr = recv(clientSocket, tr_buffer, sizeof(tr_buffer) - 1, 0);
                        if (rec_tr <= 0)
                        {
                            perror("Failed to receive transfer confirmation");
                            break;
                        }
                        tr_buffer[rec_tr] = '\0';
                        printf("%s\n", tr_buffer);
                        break;
                    }

                    case 5: // Apply for Loan
                    {
                        printf("Enter desired loan amount: ");
                        int loan_amount;
                        if (scanf("%d", &loan_amount) != 1)
                        {
                            loan_amount = 0;
                        }
                        if (send(clientSocket, &loan_amount, sizeof(loan_amount), 0) <= 0)
                        {
                            perror("Failed to send loan amount");
                            break;
                        }
                        char loan_buff[1024];
                        memset(loan_buff, 0, sizeof(loan_buff));
                        int rec_loan = recv(clientSocket, loan_buff, sizeof(loan_buff) - 1, 0);
                        if (rec_loan <= 0)
                        {
                            perror("Failed to receive loan confirmation");
                            break;
                        }
                        loan_buff[rec_loan] = '\0';
                        printf("%s\n", loan_buff);
                        break;
                    }

                    case 6: // Change Password
                    {
                        printf("Please enter your new password: ");
                        char new_pswd1[1024];
                        if (scanf("%1023s", new_pswd1) != 1)
                            new_pswd1[0] = '\0';
                        if (send(clientSocket, new_pswd1, strlen(new_pswd1) + 1, 0) <= 0)
                        {
                            perror("Failed to send new password");
                            break;
                        }
                        char rec_pswd_buff[1024];
                        memset(rec_pswd_buff, 0, sizeof(rec_pswd_buff));
                        int rec_new_pswd1 = recv(clientSocket, rec_pswd_buff, sizeof(rec_pswd_buff) - 1, 0);
                        if (rec_new_pswd1 <= 0)
                        {
                            perror("Failed to receive password change confirmation");
                            break;
                        }
                        rec_pswd_buff[rec_new_pswd1] = '\0';
                        printf("%s\n", rec_pswd_buff);
                        break;
                    }

                    case 7: // Add Feedback
                    {
                        printf("Please provide your feedback: \n");
                        char feed[256];
                        int c;
                        while ((c = getchar()) != '\n' && c != EOF) {} // Flush newline from previous input
                        if (fgets(feed, sizeof(feed), stdin) == NULL)
                            feed[0] = '\0';
                        
                        // Trim trailing newline from fgets
                        size_t ln = strlen(feed);
                        if (ln > 0 && feed[ln - 1] == '\n')
                            feed[ln - 1] = '\0';

                        if (send(clientSocket, feed, strlen(feed) + 1, 0) <= 0)
                        {
                            perror("Failed to send feedback");
                            break;
                        }
                        char feed_buff[1024];
                        memset(feed_buff, 0, sizeof(feed_buff));
                        int rec_feed = recv(clientSocket, feed_buff, sizeof(feed_buff) - 1, 0);
                        if (rec_feed <= 0)
                        {
                            perror("Failed to receive feedback confirmation");
                            break;
                        }
                        feed_buff[rec_feed] = '\0';
                        printf("%s\n", feed_buff);
                        break;
                    }

                    case 8: // View Transaction History
                        show_transaction_history(userid);
                        break;

                    case 9: // Logout
                    {
                        char logout_buff[1024];
                        memset(logout_buff, 0, sizeof(logout_buff));
                        int rec_logout = recv(clientSocket, logout_buff, sizeof(logout_buff) - 1, 0);
                        if (rec_logout <= 0)
                        {
                            perror("Failed to receive logout confirmation");
                            break;
                        }
                        logout_buff[rec_logout] = '\0';
                        printf("%s\n", logout_buff);
                        break;
                    }

                    case 10: // Exit
                    {
                        char buffer_cust[100];
                        memset(buffer_cust, 0, sizeof(buffer_cust));
                        int rcv_cust = recv(clientSocket, buffer_cust, sizeof(buffer_cust) - 1, 0);
                        if (rcv_cust <= 0)
                        {
                            perror("Failed to receive exit message");
                            break;
                        }
                        buffer_cust[rcv_cust] = '\0';
                        printf("%s\n", buffer_cust);
                        exit(0);
                    }

                    default:
                        printf("Invalid selection. Please try again.\n");
                        break;
                    }
                    if (user_choice == 9) // Break from inner loop on logout
                        break;
                }
            }
            else
            {
                printf("%s\n", auth_buffer); // Print authentication failure message
            }
            break;
        }

        case 2: // Employee Functionality
        {
            printf("Username: ");
            char username_buffer[128];
            char password_buffer[128];
            int userid_buffer;

            if (scanf("%127s", username_buffer) != 1)
                username_buffer[0] = '\0';
            if (send(clientSocket, username_buffer, strlen(username_buffer) + 1, 0) <= 0)
            {
                perror("Failed to send employee username");
                break;
            }

            printf("Password: ");
            if (scanf("%127s", password_buffer) != 1)
                password_buffer[0] = '\0';
            if (send(clientSocket, password_buffer, strlen(password_buffer) + 1, 0) <= 0)
            {
                perror("Failed to send employee password");
                break;
            }

            printf("User ID: ");
            if (scanf("%d", &userid_buffer) != 1)
                userid_buffer = -1;
            if (send(clientSocket, &userid_buffer, sizeof(userid_buffer), 0) <= 0)
            {
                perror("Failed to send employee user ID");
                break;
            }

            // Await authentication response
            char auth_buffer[1000];
            memset(auth_buffer, 0, sizeof(auth_buffer));
            int rcv_auth = recv(clientSocket, auth_buffer, sizeof(auth_buffer) - 1, 0);
            if (rcv_auth <= 0)
            {
                perror("Failed to receive employee authentication");
                break;
            }
            auth_buffer[rcv_auth] = '\0';

            if (strcmp(auth_buffer, "Authentication Successful") == 0)
            {
                printf("Login successful.\n");
                while (1)
                {
                    // Employee action menu
                    printf("Choose an action (enter the corresponding number):\n"
                           "1. Add New Customer\n2. Modify Customer Details\n"
                           "3. Approve Loan\n4. View Assigned Loan Applications\n"
                           "5. View Customer Transactions (Passbook)\n6. Change Password\n"
                           "7. Logout\n8. Exit Application\n");

                    int menu;
                    if (scanf("%d", &menu) != 1)
                    {
                        int ch;
                        while ((ch = getchar()) != '\n' && ch != EOF) {}
                        continue;
                    }
                    if (send(clientSocket, &menu, sizeof(menu), 0) <= 0)
                    {
                        perror("Failed to send employee menu choice");
                        break;
                    }

                    switch (menu)
                    {
                    case 1: // Add new customer
                    {
                        printf("Enter new customer's username: ");
                        char user_name[100];
                        if (scanf("%99s", user_name) != 1)
                            user_name[0] = '\0';
                        if (send(clientSocket, user_name, strlen(user_name) + 1, 0) <= 0)
                        {
                            perror("Failed to send new customer username");
                            break;
                        }

                        printf("Enter new customer's ID: ");
                        int id;
                        if (scanf("%d", &id) != 1)
                            id = 0;
                        if (send(clientSocket, &id, sizeof(id), 0) <= 0)
                        {
                            perror("Failed to send new customer ID");
                            break;
                        }

                        printf("Enter initial balance: ");
                        int balance;
                        if (scanf("%d", &balance) != 1)
                            balance = 0;
                        if (send(clientSocket, &balance, sizeof(balance), 0) <= 0)
                        {
                            perror("Failed to send balance");
                            break;
                        }

                        printf("Enter account number: ");
                        int accno;
                        if (scanf("%d", &accno) != 1)
                            accno = 0;
                        if (send(clientSocket, &accno, sizeof(accno), 0) <= 0)
                        {
                            perror("Failed to send account number");
                            break;
                        }

                        printf("Enter password for new customer: ");
                        char password_new[100];
                        if (scanf("%99s", password_new) != 1)
                            password_new[0] = '\0';
                        if (send(clientSocket, password_new, strlen(password_new) + 1, 0) <= 0)
                        {
                            perror("Failed to send new customer password");
                            break;
                        }

                        printf("Enter contact number for new customer: ");
                        int contact_num;
                        if (scanf("%d", &contact_num) != 1)
                            contact_num = 0;
                        if (send(clientSocket, &contact_num, sizeof(contact_num), 0) <= 0)
                        {
                            perror("Failed to send contact number");
                            break;
                        }

                        printf("Enter address for new customer: ");
                        char address_new[100];
                        if (scanf("%99s", address_new) != 1)
                            address_new[0] = '\0';
                        if (send(clientSocket, address_new, strlen(address_new) + 1, 0) <= 0)
                        {
                            perror("Failed to send address");
                            break;
                        }

                        int a;
                        if (recv(clientSocket, &a, sizeof(a), 0) <= 0)
                        {
                            perror("Failed to receive add customer response");
                            break;
                        }
                        if (a == 0)
                            printf("Error: Customer ID or account number is already in use.\n");
                        else
                            printf("New customer added successfully.\n");
                        break;
                    }

                    case 2: // Modify customer details
                    {
                        printf("Enter the ID of the customer to modify: ");
                        int id_;
                        if (scanf("%d", &id_) != 1)
                            id_ = 0;
                        if (send(clientSocket, &id_, sizeof(id_), 0) <= 0)
                        {
                            perror("Failed to send customer ID for modification");
                            break;
                        }

                        printf("Select detail to update:\n1. Name\n2. Contact\n3. Address\n");
                        int choice;
                        if (scanf("%d", &choice) != 1)
                            choice = 0;
                        if (send(clientSocket, &choice, sizeof(choice), 0) <= 0)
                        {
                            perror("Failed to send modification choice");
                            break;
                        }
                        switch (choice)
                        {
                        case 1:
                            printf("Enter new customer name: ");
                            char name[256];
                            if (scanf("%255s", name) != 1)
                                name[0] = '\0';
                            if (send(clientSocket, name, strlen(name) + 1, 0) <= 0)
                            {
                                perror("Failed to send new name");
                            }
                            break;
                        case 2:
                            printf("Enter new contact number: ");
                            int contact2;
                            if (scanf("%d", &contact2) != 1)
                                contact2 = 0;
                            if (send(clientSocket, &contact2, sizeof(contact2), 0) <= 0)
                            {
                                perror("Failed to send new contact");
                            }
                            break;
                        case 3:
                            printf("Enter new address for customer: ");
                            char address2[100];
                            if (scanf("%99s", address2) != 1)
                                address2[0] = '\0';
                            if (send(clientSocket, address2, strlen(address2) + 1, 0) <= 0)
                            {
                                perror("Failed to send new address");
                            }
                            break;
                        default:
                            printf("Invalid selection.\n");
                            break;
                        }

                        char s[200];
                        memset(s, 0, sizeof(s));
                        int rec = recv(clientSocket, s, sizeof(s) - 1, 0);
                        if (rec <= 0)
                        {
                            perror("Failed to receive modification response");
                            break;
                        }
                        s[rec] = '\0';
                        if (strcmp(s, "record updated.") == 0)
                        {
                            printf("Record updated successfully.\n");
                        }
                        else
                        {
                            printf("Failed to update user details.\n");
                        }
                        break;
                    }
                    case 3: // Approve loan
                    {
                        printf("Enter the customer ID for loan approval: ");
                        int user;
                        if (scanf("%d", &user) != 1)
                            user = 0;
                        if (send(clientSocket, &user, sizeof(user), 0) <= 0)
                        {
                            perror("Failed to send loan approval ID");
                            break;
                        }
                        char buffer2[1024];
                        memset(buffer2, 0, sizeof(buffer2));
                        int rec = recv(clientSocket, buffer2, sizeof(buffer2) - 1, 0);
                        if (rec <= 0)
                        {
                            perror("Failed to receive loan approval response");
                            break;
                        }
                        buffer2[rec] = '\0';
                        printf("%s", buffer2);
                        break;
                    }
                    case 4: // View Assigned Loan Applications
                        view_assigned_loan_applications(userid_buffer);
                        break;
                    case 5: // View Customer Transactions
                    {
                        printf("Enter customer ID: ");
                        int c_id;
                        if (scanf("%d", &c_id) != 1)
                            c_id = 0;
                        show_transaction_history(c_id);
                        break;
                    }
                    case 6: // Change Password
                    {
                        printf("Enter your new password: ");
                        char buff[256];
                        if (scanf("%255s", buff) != 1)
                            buff[0] = '\0';
                        if (send(clientSocket, buff, strlen(buff) + 1, 0) <= 0)
                        {
                            perror("Failed to send employee's new password");
                            break;
                        }
                        char s_[200];
                        memset(s_, 0, sizeof(s_));
                        int rcv = recv(clientSocket, s_, sizeof(s_) - 1, 0);
                        if (rcv <= 0)
                        {
                            perror("Failed to receive password change confirmation");
                            break;
                        }
                        s_[rcv] = '\0';
                        printf("%s", s_);
                        break;
                    }
                    case 7: // Logout
                    {
                        char buffer3[1024];
                        memset(buffer3, 0, sizeof(buffer3));
                        int rec_ = recv(clientSocket, buffer3, sizeof(buffer3) - 1, 0);
                        if (rec_ <= 0)
                        {
                            perror("Failed to receive employee logout confirmation");
                            break;
                        }
                        buffer3[rec_] = '\0';
                        printf("%s", buffer3);
                        break;
                    }
                    case 8: // Exit
                    {
                        char buffer_emp[100];
                        memset(buffer_emp, 0, sizeof(buffer_emp));
                        int rcv_emp = recv(clientSocket, buffer_emp, sizeof(buffer_emp) - 1, 0);
                        if (rcv_emp <= 0)
                        {
                            perror("Failed to receive exit message");
                            break;
                        }
                        buffer_emp[rcv_emp] = '\0';
                        printf("%s\n", buffer_emp);
                        exit(0);
                    }
                    default:
                        break;
                    }
                    if (menu == 7) // Break from inner loop on logout
                        break;
                }
            }
            else
            {
                printf("%s\n", auth_buffer); // Print authentication failure message
            }
            break;
        }

        case 3: // Manager Functionality
        {
            printf("Username: ");
            if (scanf("%1023s", username) != 1)
                username[0] = '\0';
            if (send(clientSocket, username, strlen(username) + 1, 0) <= 0)
            {
                perror("Failed to send manager username");
                break;
            }
            printf("Password: ");
            if (scanf("%1023s", password) != 1)
                password[0] = '\0';
            if (send(clientSocket, password, strlen(password) + 1, 0) <= 0)
            {
                perror("Failed to send manager password");
                break;
            }
            printf("User ID: ");
            if (scanf("%d", &userid) != 1)
                userid = -1;
            if (send(clientSocket, &userid, sizeof(userid), 0) <= 0)
            {
                perror("Failed to send manager user ID");
                break;
            }

            char auth_buffer[1024];
            memset(auth_buffer, 0, sizeof(auth_buffer));
            int auth_rcv_ = recv(clientSocket, auth_buffer, sizeof(auth_buffer) - 1, 0);
            if (auth_rcv_ <= 0)
            {
                perror("Failed to receive manager authentication");
                break;
            }
            auth_buffer[auth_rcv_] = '\0';
            if (strcmp(auth_buffer, "Authentication Successful") == 0)
            {
                printf("Login successful.\n");
                while (1)
                {
                    printf("Choose an action (enter the corresponding number):\n"
                           "1. Deactivate Account\n2. Show All Loan Applications\n"
                           "3. Assign Loan Application\n4. Review Customer Feedback\n"
                           "5. Change Password\n6. Logout\n7. Exit Application\n");
                    int choice;
                    if (scanf("%d", &choice) != 1)
                    {
                        int ch;
                        while ((ch = getchar()) != '\n' && ch != EOF) {}
                        continue;
                    }
                    if (send(clientSocket, &choice, sizeof(choice), 0) <= 0)
                    {
                        perror("Failed to send manager choice");
                        break;
                    }
                    switch (choice)
                    {
                    case 1: // Deactivate account
                    {
                        printf("Enter the ID of the customer account to deactivate: ");
                        int cust_id;
                        if (scanf("%d", &cust_id) != 1)
                            cust_id = 0;
                        if (send(clientSocket, &cust_id, sizeof(cust_id), 0) <= 0)
                        {
                            perror("Failed to send deactivation ID");
                        }
                        break;
                    }
                    case 2: // Show all applied loan applications
                        view_applied_loan_applications();
                        break;
                    case 3: // Assign loan application to employee
                    {
                        printf("Enter the employee ID to assign the loan application to: ");
                        int emp_id_loan;
                        if (scanf("%d", &emp_id_loan) != 1)
                            emp_id_loan = 0;
                        if (send(clientSocket, &emp_id_loan, sizeof(emp_id_loan), 0) <= 0)
                        {
                            perror("Failed to send assignment employee ID");
                            break;
                        }
                        printf("Enter the customer ID whose loan you are assigning: ");
                        int cust_id_loan;
                        if (scanf("%d", &cust_id_loan) != 1)
                            cust_id_loan = 0;
                        if (send(clientSocket, &cust_id_loan, sizeof(cust_id_loan), 0) <= 0)
                        {
                            perror("Failed to send assignment customer ID");
                            break;
                        }
                        char loan_buff[256];
                        memset(loan_buff, 0, sizeof(loan_buff));
                        int rec_loan = recv(clientSocket, loan_buff, sizeof(loan_buff) - 1, 0);
                        if (rec_loan <= 0)
                        {
                            perror("Failed to receive loan assignment response");
                            break;
                        }
                        loan_buff[rec_loan] = '\0';
                        printf("%s", loan_buff);
                        break;
                    }
                    case 4: // Review Feedback
                    {
                        printf("Enter customer ID to review their feedback: ");
                        int cust_id_feed;
                        if (scanf("%d", &cust_id_feed) != 1)
                            cust_id_feed = 0;
                        send(clientSocket, &cust_id_feed, sizeof(cust_id_feed), 0);

                        char feedback_msg[512];
                        memset(feedback_msg, 0, sizeof(feedback_msg));
                        int recv_feedback = recv(clientSocket, feedback_msg, sizeof(feedback_msg) - 1, 0);

                        if (recv_feedback > 0)
                        {
                            feedback_msg[recv_feedback] = '\0';
                            printf("%s\n", feedback_msg);

                            // Prompt for resolution only if there is feedback to resolve
                            if (strstr(feedback_msg, "No unresolved") == NULL)
                            {
                                printf("Mark feedback as resolved? (Y/N): ");
                                char resp;
                                scanf(" %c", &resp);
                                send(clientSocket, &resp, sizeof(resp), 0);

                                char confirm_msg[256];
                                memset(confirm_msg, 0, sizeof(confirm_msg));
                                int rec_conf = recv(clientSocket, confirm_msg, sizeof(confirm_msg) - 1, 0);
                                if (rec_conf > 0)
                                {
                                    confirm_msg[rec_conf] = '\0';
                                    printf("%s\n", confirm_msg);
                                }
                            }
                        }
                        else
                        {
                            printf("Could not retrieve feedback or connection lost.\n");
                        }
                        break;
                    }
                    case 5: // Change password
                    {
                        printf("Enter your new password: ");
                        char new_pass[256];
                        if (scanf("%255s", new_pass) != 1)
                            new_pass[0] = '\0';
                        if (send(clientSocket, new_pass, strlen(new_pass) + 1, 0) <= 0)
                        {
                            perror("Failed to send manager's new password");
                            break;
                        }
                        char pass_buff[1024];
                        memset(pass_buff, 0, sizeof(pass_buff));
                        int rec_new_pass = recv(clientSocket, pass_buff, sizeof(pass_buff) - 1, 0);
                        if (rec_new_pass <= 0)
                        {
                            perror("Failed to receive password change confirmation");
                            break;
                        }
                        pass_buff[rec_new_pass] = '\0';
                        printf("%s", pass_buff);
                        break;
                    }
                    case 6: // Logout
                    {
                        char logout_manager_buff[1024];
                        memset(logout_manager_buff, 0, sizeof(logout_manager_buff));
                        int rec_logout_ = recv(clientSocket, logout_manager_buff, sizeof(logout_manager_buff) - 1, 0);
                        if (rec_logout_ <= 0)
                        {
                            perror("Failed to receive manager logout confirmation");
                            break;
                        }
                        logout_manager_buff[rec_logout_] = '\0';
                        printf("%s", logout_manager_buff);
                        break;
                    }
                    case 7: // Exit
                    {
                        char buffer_mngr[100];
                        memset(buffer_mngr, 0, sizeof(buffer_mngr));
                        int rcv_mngr = recv(clientSocket, buffer_mngr, sizeof(buffer_mngr) - 1, 0);
                        if (rcv_mngr <= 0)
                        {
                            perror("Failed to receive exit message");
                            break;
                        }
                        buffer_mngr[rcv_mngr] = '\0';
                        printf("%s\n", buffer_mngr);
                        exit(0);
                    }
                    default:
                    {
                        char rec_def_buff[1024];
                        memset(rec_def_buff, 0, sizeof(rec_def_buff));
                        int rec_def = recv(clientSocket, rec_def_buff, sizeof(rec_def_buff) - 1, 0);
                        if (rec_def <= 0)
                        {
                            perror("Failed to receive default response");
                            break;
                        }
                        rec_def_buff[rec_def] = '\0';
                        printf("%s", rec_def_buff);
                        break;
                    }
                    }
                    if (choice == 6) // Break from inner loop on logout
                        break;
                }
            }
            else
            {
                printf("%s\n", auth_buffer); // Print authentication failure message
            }
            break;
        }

        case 4: // Admin Functionality
        {
            printf("Username: ");
            char name_buf[256];
            if (scanf("%255s", name_buf) != 1)
                name_buf[0] = '\0';
            if (send(clientSocket, name_buf, strlen(name_buf) + 1, 0) <= 0)
            {
                perror("Failed to send admin username");
                break;
            }

            printf("Password: ");
            char pass_buf[256];
            if (scanf("%255s", pass_buf) != 1)
                pass_buf[0] = '\0';
            if (send(clientSocket, pass_buf, strlen(pass_buf) + 1, 0) <= 0)
            {
                perror("Failed to send admin password");
                break;
            }

            printf("User ID: ");
            int id_buf;
            if (scanf("%d", &id_buf) != 1)
                id_buf = -1;
            if (send(clientSocket, &id_buf, sizeof(id_buf), 0) <= 0)
            {
                perror("Failed to send admin ID");
                break;
            }

            char auth_buff[128];
            memset(auth_buff, 0, sizeof(auth_buff));
            int rec_auth_buff = recv(clientSocket, auth_buff, sizeof(auth_buff) - 1, 0);
            if (rec_auth_buff <= 0)
            {
                perror("Failed to receive admin authentication");
                break;
            }
            auth_buff[rec_auth_buff] = '\0';
            if (strcmp("admin authenticated", auth_buff) == 0 || strcmp("Authentication Successful", auth_buff) == 0)
            {
                printf("Login successful.\n");
                while (1)
                {
                    printf("Choose an action (enter the corresponding number):\n"
                           "1. Add New Bank Employee\n2. Modify Customer Details\n3. Modify Employee Details\n"
                           "4. Promote Employee to Manager\n5. Change Password\n6. Logout\n7. Exit Application\n");
                    int choice;
                    if (scanf("%d", &choice) != 1)
                    {
                        int ch;
                        while ((ch = getchar()) != '\n' && ch != EOF) {}
                        continue;
                    }

                    if (send(clientSocket, &choice, sizeof(choice), 0) <= 0)
                    {
                        perror("Failed to send admin choice");
                        break;
                    }

                    switch (choice)
                    {
                    case 1: // Add New Bank Employee
                    {
                        printf("Enter new employee's username: ");
                        char emp_name[256];
                        if (scanf("%255s", emp_name) != 1)
                            emp_name[0] = '\0';
                        if (send(clientSocket, emp_name, strlen(emp_name) + 1, 0) <= 0)
                        {
                            perror("Failed to send new employee name");
                            break;
                        }

                        printf("Enter employee ID: ");
                        int emp_id;
                        if (scanf("%d", &emp_id) != 1)
                            emp_id = 0;
                        if (send(clientSocket, &emp_id, sizeof(emp_id), 0) <= 0)
                        {
                            perror("Failed to send new employee ID");
                            break;
                        }

                        printf("Enter password for new employee: ");
                        char emp_pass[256];
                        if (scanf("%255s", emp_pass) != 1)
                            emp_pass[0] = '\0';
                        if (send(clientSocket, emp_pass, strlen(emp_pass) + 1, 0) <= 0)
                        {
                            perror("Failed to send new employee password");
                            break;
                        }

                        char buff_resp[256];
                        memset(buff_resp, 0, sizeof(buff_resp));
                        int rcv = recv(clientSocket, buff_resp, sizeof(buff_resp) - 1, 0);
                        if (rcv <= 0)
                        {
                            perror("Failed to receive add employee response");
                            break;
                        }
                        buff_resp[rcv] = '\0';
                        if (strcmp(buff_resp, "employee already exists.") == 0)
                        {
                            printf("This employee already exists.\n");
                        }
                        else
                        {
                            printf("New employee added successfully.\n");
                        }
                        break;
                    }
                    case 2: // Modify Customer
                    {
                        printf("Enter the ID of the customer to modify: ");
                        int id2;
                        if (scanf("%d", &id2) != 1)
                            id2 = 0;
                        if (send(clientSocket, &id2, sizeof(id2), 0) <= 0)
                        {
                            perror("Failed to send customer ID for modification");
                            break;
                        }

                        printf("Select detail to update:\n1. Name\n2. Contact\n3. Address\n");
                        int ch_key;
                        if (scanf("%d", &ch_key) != 1)
                            ch_key = 0;
                        if (send(clientSocket, &ch_key, sizeof(ch_key), 0) <= 0)
                        {
                            perror("Failed to send modification key");
                            break;
                        }
                        switch (ch_key)
                        {
                        case 1:
                            printf("Enter new customer name: ");
                            char new_name[256];
                            if (scanf("%255s", new_name) != 1)
                                new_name[0] = '\0';
                            if (send(clientSocket, new_name, strlen(new_name) + 1, 0) <= 0)
                            {
                                perror("Failed to send new name");
                            }
                            break;
                        case 2:
                            printf("Enter new contact number: ");
                            int contact_new;
                            if (scanf("%d", &contact_new) != 1)
                                contact_new = 0;
                            if (send(clientSocket, &contact_new, sizeof(contact_new), 0) <= 0)
                            {
                                perror("Failed to send new contact");
                            }
                            break;
                        case 3:
                            printf("Enter new address for customer: ");
                            char address_new[100];
                            if (scanf("%99s", address_new) != 1)
                                address_new[0] = '\0';
                            if (send(clientSocket, address_new, strlen(address_new) + 1, 0) <= 0)
                            {
                                perror("Failed to send new address");
                            }
                            break;
                        default:
                            printf("Invalid selection.\n");
                            break;
                        }

                        char s[200];
                        memset(s, 0, sizeof(s));
                        int rcv2 = recv(clientSocket, s, sizeof(s) - 1, 0);
                        if (rcv2 <= 0)
                        {
                            perror("Failed to receive admin modification response");
                            break;
                        }
                        s[rcv2] = '\0';
                        if (strcmp(s, "record updated.") == 0)
                        {
                            printf("Record updated successfully.\n");
                        }
                        else
                        {
                            printf("Failed to update user details.\n");
                        }
                        break;
                    }
                    case 3: // Modify Employee
                    {
                        printf("Enter the ID of the employee to modify: ");
                        int id3;
                        if (scanf("%d", &id3) != 1)
                            id3 = 0;
                        if (send(clientSocket, &id3, sizeof(id3), 0) <= 0)
                        {
                            perror("Failed to send employee ID for modification");
                            break;
                        }

                        printf("Enter new employee name: ");
                        char buff2[256];
                        if (scanf("%255s", buff2) != 1)
                            buff2[0] = '\0';
                        if (send(clientSocket, buff2, strlen(buff2) + 1, 0) <= 0)
                        {
                            perror("Failed to send new employee name");
                            break;
                        }
                        char buu_em[200];
                        memset(buu_em, 0, sizeof(buu_em));
                        int recm = recv(clientSocket, buu_em, sizeof(buu_em) - 1, 0);
                        if (recm <= 0)
                        {
                            perror("Failed to receive employee modification response");
                            break;
                        }
                        buu_em[recm] = '\0';
                        if (recm <= 0)
                        {
                            perror("Invalid ID provided");
                            return 1;
                        }
                        break;
                    }
                    case 4: // Promote employee to manager
                    {
                        printf("Enter the ID of the employee to promote to manager: ");
                        int id5;
                        if (scanf("%d", &id5) != 1)
                            id5 = 0;
                        if (send(clientSocket, &id5, sizeof(id5), 0) <= 0)
                        {
                            perror("Failed to send promotion ID");
                            break;
                        }
                        char s2[200];
                        memset(s2, 0, sizeof(s2));
                        int rcv5 = recv(clientSocket, s2, sizeof(s2) - 1, 0);
                        if (rcv5 <= 0)
                        {
                            perror("Failed to receive promotion response");
                            break;
                        }
                        s2[rcv5] = '\0';
                        printf("%s", s2);
                        break;
                    }
                    case 5: // Change password
                    {
                        printf("Enter your new password: ");
                        char buff3[256];
                        if (scanf("%255s", buff3) != 1)
                            buff3[0] = '\0';
                        if (send(clientSocket, buff3, strlen(buff3) + 1, 0) <= 0)
                        {
                            perror("Failed to send admin's new password");
                            break;
                        }
                        char s3[200];
                        memset(s3, 0, sizeof(s3));
                        int rcv6 = recv(clientSocket, s3, sizeof(s3) - 1, 0);
                        if (rcv6 <= 0)
                        {
                            perror("Failed to receive password change confirmation");
                            break;
                        }
                        s3[rcv6] = '\0';
                        printf("%s", s3);
                        break;
                    }
                    case 6: // Logout
                    {
                        char s4[200];
                        memset(s4, 0, sizeof(s4));
                        int rcv7 = recv(clientSocket, s4, sizeof(s4) - 1, 0);
                        if (rcv7 <= 0)
                        {
                            perror("Failed to receive admin logout confirmation");
                            break;
                        }
                        s4[rcv7] = '\0';
                        printf("%s", s4);
                        break;
                    }
                    case 7: // Exit
                    {
                        char buffer_admin[100];
                        memset(buffer_admin, 0, sizeof(buffer_admin));
                        int rcv_admin = recv(clientSocket, buffer_admin, sizeof(buffer_admin) - 1, 0);
                        if (rcv_admin <= 0)
                        {
                            perror("Failed to receive exit message");
                            break;
                        }
                        buffer_admin[rcv_admin] = '\0';
                        printf("%s\n", buffer_admin);
                        exit(0);
                    }
                    default:
                        printf("Please enter a valid option.\n");
                        break;
                    }
                    if (choice == 6) // Break from inner loop on logout
                        break;
                }
            }
            else
            {
                printf("%s\n", auth_buff); // Print authentication failure message
            }
            break;
        }

        default:
            break;
        }
    }

    // Clean up and close the socket
    close(clientSocket);
    return 0;
}