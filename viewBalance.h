

#ifndef VIEWBALANCE_H
#define VIEWBALANCE_H

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

int view_balance(int client_socket, int customer_id)
{
	// Open customer database for reading
	int fd_read = open("customer_db.txt", O_RDWR);
	int current_balance = 0;
	struct customer temp_cust;
	// Search for customer in database
	while (read(fd_read, &temp_cust, sizeof(struct customer)) > 0)
	{
		if (customer_id == temp_cust.customer_id)
		{
			// Set up read lock for accessing balance
			struct flock lk;
			lk.l_type = F_RDLCK;
			lk.l_whence = SEEK_CUR;
			lk.l_start = lseek(fd_read, 0, SEEK_CUR) - sizeof(struct customer);
			lk.l_len = sizeof(struct customer);

			// Acquire lock
			if (fcntl(fd_read, F_SETLKW, &lk) == -1)
			{
				perror("error fcntl");
				close(fd_read);
				close(client_socket);
				return -1;
			}

			// Read balance
			current_balance = temp_cust.balance;

			// Release lock
			lk.l_type = F_UNLCK;
			if (fcntl(fd_read, F_SETLK, &lk) == -1)
			{
				perror("error fcntl");
				close(fd_read);
				close(client_socket);
				return -1;
			}

			// Send balance to client
			char msg[256];
			sprintf(msg, "Current balance: %d\n", current_balance);
			send(client_socket, msg, strlen(msg), 0);
			close(fd_read);
			return current_balance;
		}
	}

	// Customer not found
	send(client_socket, "Could not find customer.\n", strlen("Could not find customer.\n"), 0);
	return current_balance;
}
#endif