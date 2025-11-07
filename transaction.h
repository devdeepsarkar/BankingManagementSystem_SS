#ifndef TRANSACTION_H
#define TRANSACTION_H

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
#include "transactionHistory.h"


void deposit(int client_socket, int customer_id, int add_balance)
{
	// Open customer database
	int fd_read = open("customer_db.txt", O_RDWR);
	if (fd_read == -1)
	{
		perror("open error");
		return;
	}

	struct customer temp_cust;
	while (read(fd_read, &temp_cust, sizeof(struct customer)) > 0)
	{
		if (customer_id == temp_cust.customer_id)
		{
			// Set up write lock for balance update
			struct flock lk;
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_CUR;
			lk.l_start = lseek(fd_read, 0, SEEK_CUR) - sizeof(struct customer);
			lk.l_len = sizeof(struct customer);

			// Acquire lock
			if (fcntl(fd_read, F_SETLKW, &lk) == -1)
			{
				perror("error fcntl");
				close(fd_read);
				return;
			}

			// Process deposit
			int old_balance = temp_cust.balance;
			temp_cust.balance = temp_cust.balance + add_balance;

			// Log transaction
			char *transaction_type = "Deposit: ";
			log_transaction(client_socket, customer_id, add_balance, transaction_type);

			// Update customer record
			if (lseek(fd_read, -sizeof(struct customer), SEEK_CUR) == -1)
			{
				perror("lseek error");
				return;
			}
			if ((write(fd_read, &temp_cust, sizeof(struct customer))) <= 0)
			{
				perror("write error");
				return;
			}

			// Release lock
			lk.l_type = F_UNLCK;
			if (fcntl(fd_read, F_SETLK, &lk) == -1)
			{
				perror("error fcntl");
				close(fd_read);
				return;
			}

			// Send success message
			char msg[256];
			sprintf(msg, "Balance updated.\nOld balance: %d\nNew balance: %d",
					old_balance, temp_cust.balance);
			send(client_socket, msg, strlen(msg), 0);
			close(fd_read);
			return;
		}
	}

	send(client_socket, "Could not find customer id in db",
		 strlen("Could not find customer id in db"), 0);
	close(fd_read);
}


void withdraw(int client_socket, int customer_id, int withdraw_amount)
{
	// Open customer database
	int fd_read = open("customer_db.txt", O_RDWR);
	struct customer temp_cust;

	while (read(fd_read, &temp_cust, sizeof(struct customer)) > 0)
	{
		if (customer_id == temp_cust.customer_id)
		{
			// Set up write lock for balance update
			struct flock lk;
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_CUR;
			lk.l_start = lseek(fd_read, 0, SEEK_CUR) - sizeof(struct customer);
			lk.l_len = sizeof(struct customer);

			// Acquire lock
			if (fcntl(fd_read, F_SETLKW, &lk) == -1)
			{
				perror("error fcntl");
				close(fd_read);
				return;
			}

			// Check for sufficient balance
			int old_balance = temp_cust.balance;
			if (old_balance < withdraw_amount)
			{
				char msg[256];
				sprintf(msg, "Not enough balance.Current balance: %d", old_balance);
				send(client_socket, msg, strlen(msg), 0);
				close(fd_read);
				return;
			}

			// Process withdrawal
			temp_cust.balance = temp_cust.balance - withdraw_amount;

			// Log transaction
			char *transaction_type = "Withdraw: ";
			log_transaction(client_socket, customer_id, withdraw_amount, transaction_type);

			// Update customer record
			if (lseek(fd_read, -sizeof(struct customer), SEEK_CUR) == -1)
			{
				perror("lseek error");
				return;
			}
			if ((write(fd_read, &temp_cust, sizeof(struct customer))) <= 0)
			{
				perror("write error");
				return;
			}

			// Release lock
			lk.l_type = F_UNLCK;
			if (fcntl(fd_read, F_SETLK, &lk) == -1)
			{
				perror("error fcntl");
				close(fd_read);
				return;
			}

			// Send success message
			char msg[256];
			sprintf(msg, "Balance updated.\nOld balance: %d\nNew balance: %d",
					old_balance, temp_cust.balance);
			send(client_socket, msg, strlen(msg), 0);
			close(fd_read);
			return;
		}
	}

	send(client_socket, "Could not find customer id in db",
		 strlen("Could not find customer id in db"), 0);
	close(fd_read);
}


void transfer_fund(int client_socket, int sender_customer_id, int receiver_customer_id,
				   int transfer_amount)
{
	if (sender_customer_id == receiver_customer_id)
	{
		char msg[256];
		sprintf(msg, "Transaction Failed.\nSender's and receiver's ID cannot be the same.\n");
		send(client_socket, msg, strlen(msg), 0);
		return;
	}

	// Open customer database
	int fd_read = open("customer_db.txt", O_RDWR);
	struct customer sender_id, receiver_id, temp;

	// Flags for tracking account lookup
	bool sender_found = false;
	bool receiver_found = false;
	off_t sender_offset, receiver_offset;

	// Lock structures for both accounts
	struct flock lks; // lock for sender
	struct flock lkr; // lock for receiver

	// Find both accounts and set up locks
	while (read(fd_read, &temp, sizeof(struct customer)) > 0)
	{
		// Find and lock sender's account
		if (temp.customer_id == sender_customer_id)
		{
			sender_id = temp;
			lks.l_type = F_WRLCK;
			lks.l_whence = SEEK_SET;
			lks.l_start = lseek(fd_read, 0, SEEK_CUR) - sizeof(struct customer);
			lks.l_len = sizeof(struct customer);

			if (fcntl(fd_read, F_SETLKW, &lks) == -1)
			{
				perror("fcntl error");
				close(fd_read);
				return;
			}

			sender_found = true;
			sender_offset = lseek(fd_read, 0, SEEK_CUR) - sizeof(struct customer);
			if (sender_offset == -1)
			{
				perror("lseek error");
				close(fd_read);
				return;
			}
		}

		// Find and lock receiver's account
		if (temp.customer_id == receiver_customer_id)
		{
			receiver_id = temp;
			lkr.l_type = F_WRLCK;
			lkr.l_whence = SEEK_SET;
			lkr.l_start = lseek(fd_read, 0, SEEK_CUR) - sizeof(struct customer);
			lkr.l_len = sizeof(struct customer);

			if (fcntl(fd_read, F_SETLKW, &lkr) == -1)
			{
				perror("fcntl error");
				close(fd_read);
				return;
			}

			receiver_found = true;
			receiver_offset = lseek(fd_read, 0, SEEK_CUR) - sizeof(struct customer);
			if (receiver_offset == -1)
			{
				perror("lseek error");
				close(fd_read);
				return;
			}
		}
	}

	// Process transfer if both accounts found
	if (sender_found && receiver_found)
	{
		// Check for sufficient balance
		if (sender_id.balance < transfer_amount)
		{
			char msg[256];
			sprintf(msg, "Not enough balance.Current balance: %d", sender_id.balance);
			send(client_socket, msg, strlen(msg), 0);
			close(fd_read);
			return;
		}

		// Update balances
		sender_id.balance -= transfer_amount;
		receiver_id.balance += transfer_amount;

		// Update sender's record
		if (lseek(fd_read, sender_offset, SEEK_SET) == -1)
		{
			perror("lseek error");
			return;
		}
		if ((write(fd_read, &sender_id, sizeof(struct customer))) <= 0)
		{
			perror("write error");
			return;
		}

		// Update receiver's record
		if (lseek(fd_read, receiver_offset, SEEK_SET) == -1)
		{
			perror("lseek error");
			return;
		}
		if ((write(fd_read, &receiver_id, sizeof(struct customer))) <= 0)
		{
			perror("write error");
			return;
		}

		// Log transactions for both accounts
		char transaction_type[256];
		sprintf(transaction_type, "Transferred amount %d to %d",
				transfer_amount, receiver_customer_id);
		log_transaction(client_socket, sender_customer_id, transfer_amount, transaction_type);

		sprintf(transaction_type, "Received amount %d from %d",
				transfer_amount, sender_customer_id);
		log_transaction(client_socket, receiver_customer_id, transfer_amount, transaction_type);
	}

	// Release locks
	lks.l_type = F_UNLCK;
	lkr.l_type = F_UNLCK;

	if (fcntl(fd_read, F_SETLK, &lks) == -1 || fcntl(fd_read, F_SETLK, &lkr) == -1)
	{
		perror("fcntl error");
		close(fd_read);
		return;
	}

	char msg[256];
	sprintf(msg, "Amount transferred successfully!\nSender's balance: %d\nReceiver's balance: %d\n", sender_id.balance, receiver_id.balance);
	send(client_socket, msg, strlen(msg), 0);
	close(fd_read);
}

#endif // TRANSACTION_H