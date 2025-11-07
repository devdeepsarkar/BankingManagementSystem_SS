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

bool handle_customer(int client_socket)
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
			fprintf(stderr, "Client closed before customer username recv\n");
		else
			perror("recv error customer username");
		return false;
	}
	username_buffer[rec_client] = '\0';

	/* Receive password */
	rec_client = recv(client_socket, password_buffer, sizeof(password_buffer) - 1, 0);
	if (rec_client <= 0)
	{
		if (rec_client == 0)
			fprintf(stderr, "Client closed before customer password recv\n");
		else
			perror("recv error customer password");
		return false;
	}
	password_buffer[rec_client] = '\0';

	/* Receive user id */
	rec_client = recv(client_socket, &userid_buffer, sizeof(userid_buffer), 0);
	if (rec_client <= 0)
	{
		if (rec_client == 0)
			fprintf(stderr, "Client closed before customer userid recv\n");
		else
			perror("recv error customer userid");
		return false;
	}

	/* Authenticate */
	if (!authenticate_customer(client_socket, userid_buffer, password_buffer))
	{
		send(client_socket, "Invalid credential(s).\n", strlen("Invalid credential(s).\n"), 0);
		return false;
	}

	authenticated = true;
	auth_userid = userid_buffer;
	send(client_socket, "Authentication Successful", strlen("Authentication Successful"), 0);

	/* Customer menu loop */
	while (1)
	{
		int client_rcv = recv(client_socket, &menu_options_recv, sizeof(menu_options_recv), 0);
		if (client_rcv <= 0)
		{
			if (authenticated)
				customer_logout(client_socket, auth_userid);
			return false;
		}

		switch (menu_options_recv)
		{
		case 1:
			view_balance(client_socket, userid_buffer);
			break;
		case 2:
		{
			int amount;
			int r = recv(client_socket, &amount, sizeof(amount), 0);
			if (r <= 0)
			{
				if (authenticated)
					customer_logout(client_socket, auth_userid);
				return false;
			}
			deposit(client_socket, userid_buffer, amount);
			break;
		}
		case 3:
		{
			int withdraw_amount;
			int r = recv(client_socket, &withdraw_amount, sizeof(withdraw_amount), 0);
			if (r <= 0)
			{
				if (authenticated)
					customer_logout(client_socket, auth_userid);
				return false;
			}
			withdraw(client_socket, userid_buffer, withdraw_amount);
			break;
		}
		case 4:
		{
			int receiver_customer_id;
			int r = recv(client_socket, &receiver_customer_id, sizeof(receiver_customer_id), 0);
			if (r <= 0)
			{
				if (authenticated)
					customer_logout(client_socket, auth_userid);
				return false;
			}

			int transfer_amount;
			r = recv(client_socket, &transfer_amount, sizeof(transfer_amount), 0);
			if (r <= 0)
			{
				if (authenticated)
					customer_logout(client_socket, auth_userid);
				return false;
			}

			transfer_fund(client_socket, userid_buffer, receiver_customer_id, transfer_amount);
			break;
		}
		case 5:
		{
			int loan_amount;
			int r = recv(client_socket, &loan_amount, sizeof(loan_amount), 0);
			if (r <= 0)
			{
				if (authenticated)
					customer_logout(client_socket, auth_userid);
				return false;
			}
			apply_loan(client_socket, userid_buffer, loan_amount);
			break;
		}
		case 6:
			edit_credentials_customer(client_socket, userid_buffer);
			break;
		case 7:
		{
			char feedback[1024];
			int r = recv(client_socket, feedback, sizeof(feedback) - 1, 0);
			if (r <= 0)
			{
				if (authenticated)
					customer_logout(client_socket, auth_userid);
				return false;
			}
			feedback[r] = '\0';
			add_feedback(client_socket, userid_buffer, feedback);
			break;
		}
		case 8:
			show_transaction_history(userid_buffer);
			break;
		case 9:
			customer_logout(client_socket, userid_buffer);
			return true;
		case 10:
			// send(client_socket, "Exiting..\n", strlen("Exiting..\n"), 0);
			// if (authenticated)
			// 	customer_logout(client_socket, auth_userid);
			// close(client_socket);
			// exit(0);
			send(client_socket, "Exiting...\n", strlen("Exiting...\n"), 0);
			if (authenticated)
				customer_logout(client_socket, auth_userid);
			close(client_socket);
			exit(0);
		default:
			break;
		}
	}

	if (authenticated)
		customer_logout(client_socket, auth_userid);
	return false;
}
