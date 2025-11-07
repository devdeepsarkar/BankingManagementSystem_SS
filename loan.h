#ifndef LOAN_H
#define LOAN_H

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

void assign_loan(int client_socket)
{
    // Receive employee ID for assignment
    int employee_id;
    int rec_emp = recv(client_socket, &employee_id, sizeof(employee_id), 0);
    if (rec_emp <= 0)
    {
        perror("recv error");
        return;
    }

    // Receive customer ID whose loan needs to be assigned
    int cust_id;
    int rec_cust = recv(client_socket, &cust_id, sizeof(cust_id), 0);
    if (rec_cust <= 0)
    {
        perror("recv error");
        return;
    }

    // Open loan database
    int fd_loan = open("loan_db.txt", O_RDWR);
    if (fd_loan == -1)
    {
        perror("open error");
        return;
    }

    // Search and update loan record
    struct loan temp_loan;
    while (read(fd_loan, &temp_loan, sizeof(struct loan)) > 0)
    {
        if (temp_loan.customer_id == cust_id)
        {
            // Set up write lock for updating loan record
            struct flock lk;
            lk.l_type = F_WRLCK;
            lk.l_start = lseek(fd_loan, 0, SEEK_CUR) - sizeof(temp_loan);
            lk.l_len = sizeof(temp_loan);
            lk.l_whence = SEEK_SET;

            // Acquire lock
            if (fcntl(fd_loan, F_SETLKW, &lk) == -1)
            {
                perror("Locking error in feedback resolve fun");
                close(fd_loan);
                return;
            }

            // Update and write loan record
            temp_loan.emp_id = employee_id;
            if (lseek(fd_loan, -sizeof(temp_loan), SEEK_CUR) == -1)
            {
                perror("lseek error");
                close(fd_loan);
                return;
            }
            if (write(fd_loan, &temp_loan, sizeof(temp_loan)) <= 0)
            {
                perror("write error in assign loan");
                close(fd_loan);
                return;
            }

            // Release lock
            lk.l_type = F_UNLCK;
            if (fcntl(fd_loan, F_SETLK, &lk) == -1)
            {
                perror("unlck error");
                close(fd_loan);
                return;
            }

            // Send success message
            char msg[256];
            sprintf(msg, "Loan application of customer_id %d, assigned to emp_id: %d\n", cust_id, employee_id);
            send(client_socket, msg, strlen(msg), 0);
            close(fd_loan);
            return;
        }
    }
    send(client_socket, "Could not assign loan\n", strlen("Could not assign loan\n"), 0);
}


void approve_loan(int client_socket, int employee_id, int customer_id)
{
    // Open required databases
    int fd_emp = open("employee_db.txt", O_RDWR);
    int fd_cust = open("customer_db.txt", O_RDWR);
    int fd_loan = open("loan_db.txt", O_RDWR);

    if (fd_emp == -1 || fd_cust == -1 || fd_loan == -1)
    {
        perror("open error");
        return;
    }

    struct customer temp_cust;
    struct loan temp_loan;
    struct flock lkl;
    int flag1 = 0, flag2 = 0;

    // Update customer database
    while (read(fd_cust, &temp_cust, sizeof(struct customer)) > 0)
    {
        if (temp_cust.customer_id == customer_id && temp_cust.need_loan == true)
        {
            // Set up write lock for customer record
            lkl.l_type = F_WRLCK;
            lkl.l_whence = SEEK_CUR;
            lkl.l_start = lseek(fd_cust, 0, SEEK_CUR) - sizeof(struct customer);
            lkl.l_len = sizeof(struct customer);

            // Acquire lock
            if (fcntl(fd_cust, F_SETLKW, &lkl) == -1)
            {
                perror("fcntl error");
                close(fd_cust);
                close(fd_loan);
                return;
            }

            // Update and write customer record
            temp_cust.loan_approved = true;
            if (lseek(fd_cust, -sizeof(struct customer), SEEK_CUR) == -1)
            {
                perror("lseek error");
                close(fd_cust);
                close(fd_loan);
                return;
            }
            if ((write(fd_cust, &temp_cust, sizeof(struct customer))) <= 0)
            {
                perror("write error");
                close(fd_cust);
                close(fd_loan);
                return;
            }

            // Release lock
            lkl.l_type = F_UNLCK;
            if (fcntl(fd_cust, F_SETLK, &lkl) == -1)
            {
                perror("fcntl error");
                close(fd_cust);
                close(fd_loan);
                return;
            }
            flag1 = 1;
            break;
        }
    }

    // Update loan database
    while (read(fd_loan, &temp_loan, sizeof(struct loan)) > 0)
    {
        if (temp_loan.emp_id == employee_id && temp_loan.customer_id == customer_id)
        {
            strcpy(temp_loan.status, "approved");
            if (lseek(fd_loan, -sizeof(struct loan), SEEK_CUR) == -1)
            {
                perror("lseek error");
                close(fd_cust);
                close(fd_loan);
                return;
            }
            if ((write(fd_loan, &temp_loan, sizeof(struct loan))) <= 0)
            {
                perror("write error");
                close(fd_cust);
                close(fd_loan);
                return;
            }
            flag2 = 1;
            break;
        }
    }

    // Send appropriate response
    if (flag1 == 1 && flag2 == 1)
    {
        char msg[256];
        sprintf(msg, "Loan of cutomer_id: %d,loan amount: %d approved by emp_id: %d\n",
                customer_id, temp_cust.loan_amount, employee_id);
        send(client_socket, msg, strlen(msg), 0);
    }
    else
    {
        send(client_socket, "Could not approve loan", strlen("Could not approve loan"), 0);
    }

    close(fd_cust);
    close(fd_loan);
}

void apply_loan(int client_socket, int cust_id, int loan_amount)
{
    // Open required databases
    int fd_cust = open("customer_db.txt", O_RDWR);
    int fd_loan = open("loan_db.txt", O_RDWR);

    if (fd_cust == -1 || fd_loan == -1)
    {
        perror("open error");
        return;
    }

    // Prepare new loan record
    struct customer temp_cust;
    struct loan temp_loan;
    temp_loan.customer_id = cust_id;
    temp_loan.emp_id = 0;
    temp_loan.loan_amount = loan_amount;
    strcpy(temp_loan.status, "applied");

    // Set up write lock for loan database
    struct flock lkl;
    lkl.l_type = F_WRLCK;
    lkl.l_start = 0;
    lkl.l_whence = SEEK_END;
    lkl.l_len = sizeof(struct loan);

    // Write new loan record
    if (fcntl(fd_loan, F_SETLKW, &lkl) == -1)
    {
        perror("fcntl error");
        close(fd_cust);
        close(fd_loan);
        return;
    }

    if (lseek(fd_loan, 0, SEEK_END) == -1)
    {
        perror("lseek error");
        close(fd_cust);
        close(fd_loan);
        return;
    }

    if ((write(fd_loan, &temp_loan, sizeof(struct loan))) <= 0)
    {
        perror("write error");
        close(fd_cust);
        close(fd_loan);
        return;
    }

    // Release loan database lock
    lkl.l_type = F_UNLCK;
    if (fcntl(fd_loan, F_SETLK, &lkl) == -1)
    {
        perror("fcntl error");
        close(fd_cust);
        close(fd_loan);
        return;
    }

    // Update customer database
    while (read(fd_cust, &temp_cust, sizeof(struct customer)) > 0)
    {
        if (temp_cust.customer_id == cust_id)
        {
            // Set up write lock for customer record
            lkl.l_type = F_WRLCK;
            lkl.l_whence = SEEK_CUR;
            lkl.l_start = lseek(fd_cust, 0, SEEK_CUR) - sizeof(struct customer);
            lkl.l_len = sizeof(struct customer);

            if (fcntl(fd_cust, F_SETLKW, &lkl) == -1)
            {
                perror("fcntl error");
                close(fd_cust);
                close(fd_loan);
                return;
            }

            // Update customer record
            temp_cust.need_loan = true;
            temp_cust.loan_amount = loan_amount;

            if (lseek(fd_cust, -sizeof(struct customer), SEEK_CUR) == -1)
            {
                perror("lseek error");
                close(fd_cust);
                close(fd_loan);
                return;
            }

            if ((write(fd_cust, &temp_cust, sizeof(struct customer))) <= 0)
            {
                perror("write error");
                close(fd_cust);
                close(fd_loan);
                return;
            }

            // Release customer database lock
            lkl.l_type = F_UNLCK;
            if (fcntl(fd_cust, F_SETLK, &lkl) == -1)
            {
                perror("fcntl error");
                close(fd_cust);
                close(fd_loan);
                return;
            }

            char msg[256];
            sprintf(msg, "Successfully applied for loan of amount: %d", temp_cust.loan_amount);
            send(client_socket, msg, strlen(msg), 0);
            close(fd_cust);
            close(fd_loan);
            return;
        }
    }

    send(client_socket, "Could not apply for loan\n", strlen("Could not apply for loan\n"), 0);
}


void view_assigned_loan_applications(int employee_id)
{
    int fd_loan = open("loan_db.txt", O_RDONLY);
    if (fd_loan == -1)
    {
        perror("open error");
        return;
    }

    struct loan temp_loan;
    while (read(fd_loan, &temp_loan, sizeof(struct loan)) > 0)
    {
        if (temp_loan.emp_id == employee_id)
        {
            printf("Customer_id: %d,loan amount: %d, status of loan application: %s",
                   temp_loan.customer_id, temp_loan.loan_amount, temp_loan.status);
        }
    }
    close(fd_loan);
}


void view_applied_loan_applications()
{
    int fd_loan = open("loan_db.txt", O_RDONLY);
    if (fd_loan == -1)
    {
        perror("open error");
        return;
    }

    struct loan temp_loan;
    while (read(fd_loan, &temp_loan, sizeof(struct loan)) > 0)
    {
        if (strcmp(temp_loan.status, "applied") == 0)
        {
            printf("Customer_id: %d,loan amount: %d,status of loan application: %s\n",
                   temp_loan.customer_id, temp_loan.loan_amount, temp_loan.status);
        }
    }
    close(fd_loan);
}

#endif // LOAN_H
