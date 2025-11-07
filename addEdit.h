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
#include "credentials.h"

#ifndef ADDEDIT_H
#define ADDEDIT_H

void add_customer(int client_socket)
{
	// Create new customer record
	struct customer new_customer;

	// Receive and validate customer name
	int rcv_name = recv(client_socket, new_customer.name, sizeof(new_customer.name), 0);
	if (rcv_name <= 0)
	{
		perror("rcv error");
		return;
	}
	new_customer.name[rcv_name] = '\0';

	// Receive and validate customer ID
	int cust_id_buff;
	int rcv_cusid = recv(client_socket, &cust_id_buff, sizeof(cust_id_buff), 0);
	new_customer.customer_id = cust_id_buff;
	if (rcv_cusid <= 0)
	{
		perror("rcv error");
		return;
	}

	// Receive and validate initial balance
	int balance_buff;
	int rcv_balance = recv(client_socket, &balance_buff, sizeof(balance_buff), 0);
	new_customer.balance = balance_buff;
	if (rcv_balance <= 0)
	{
		perror("rcv error");
		return;
	}

	// Receive and validate account number
	int acc_buff;
	int rcv_account_number = recv(client_socket, &acc_buff, sizeof(acc_buff), 0);
	new_customer.account_number = acc_buff;
	if (rcv_account_number <= 0)
	{
		perror("rcv error");
		return;
	}

	// Handle password: receive, hash, and store
	char temp_pswd[128];
	int rcv_password = recv(client_socket, temp_pswd, sizeof(temp_pswd), 0);
	if (rcv_password <= 0)
	{
		perror("recv error");
		return;
	}
	temp_pswd[rcv_password] = '\0';

	// Process password - trim spaces and hash
	char hashed_pswd[32];
	char hex_pswd[65];
	trim_trailing_spaces(temp_pswd);
	trim_leading_spaces(temp_pswd);
	hash_password(temp_pswd, hashed_pswd);
	password_hash_to_hex(hashed_pswd, hex_pswd);
	strcpy(new_customer.password, hex_pswd);

	// Receive and validate contact number
	int contact_num;
	int rcv_contact = recv(client_socket, &contact_num, sizeof(contact_num), 0);
	if (rcv_contact == -1)
	{
		perror("recv error");
		return;
	}
	new_customer.contact = contact_num;

	// Receive and validate address
	char addr[1024];
	int rcv_addr = recv(client_socket, addr, sizeof(addr), 0);
	if (rcv_addr == -1)
	{
		perror("recv error");
		return;
	}
	addr[rcv_addr] = '\0';
	strcpy(new_customer.address, addr);

	// Initialize default values for new customer
	new_customer.need_loan = false;
	new_customer.loan_amount = 0;
	new_customer.loan_approved = false;
	new_customer.is_online = false;
	new_customer.is_active = true;

	// Open customer database with write access
	int fd_read = open("customer_db.txt", O_RDWR);
	if (fd_read == -1)
	{
		perror("error while opening customer db");
		return;
	}

	// Check for duplicate customer ID or account number
	struct customer temp_cust;
	int flag = 0;
	while ((read(fd_read, &temp_cust, sizeof(struct customer))) > 0)
	{
		if (temp_cust.customer_id == new_customer.customer_id ||
			temp_cust.account_number == new_customer.account_number)
		{
			send(client_socket, &flag, sizeof(flag), 0);
			close(fd_read);
			return;
		}
	}

	// Set success flag and prepare to write new record
	flag = 1;

	// Apply write lock before adding new customer
	struct flock lk;
	lk.l_type = F_WRLCK;
	lk.l_whence = SEEK_END;
	lk.l_start = 0;
	lk.l_len = sizeof(struct customer);

	// Acquire lock
	if (fcntl(fd_read, F_SETLKW, &lk) == -1)
	{
		perror("locking error");
		close(fd_read);
		return;
	}

	// Position at end of file for new record
	if (lseek(fd_read, 0, SEEK_END) == -1)
	{
		perror("lseek error");
		close(fd_read);
		return;
	}

	// Write new customer record
	if ((write(fd_read, &new_customer, sizeof(struct customer))) <= 0)
	{
		perror("write error");
		close(fd_read);
		return;
	}

	// Send success response to client
	send(client_socket, &flag, sizeof(flag), 0);

	// Release lock
	lk.l_type = F_UNLCK;
	if (fcntl(fd_read, F_SETLK, &lk) == -1)
	{
		perror("locking error");
		close(fd_read);
		return;
	}
	close(fd_read);
}

void edit_customer(int client_socket)
{
	// Receive customer ID to modify
	int user_id;
	int rec_usr_id = recv(client_socket, &user_id, sizeof(user_id), 0);
	if (rec_usr_id == -1)
	{
		perror("recv error");
		return;
	}

	// Receive which field to modify
	struct customer new_customer;
	int menue_option = 0;
	int rcv_cusid = recv(client_socket, &menue_option, sizeof(menue_option), 0);

	// Handle different update options
	switch (menue_option)
	{
	case 1: // Update name
		int rcv_name = recv(client_socket, new_customer.name, sizeof(new_customer.name), 0);
		if (rcv_name <= 0)
		{
			perror("rcv error");
			return;
		}
		new_customer.name[rcv_name] = '\0';
		break;

	case 2: // Update contact
		int rcv_contact = recv(client_socket, &new_customer.contact, sizeof(new_customer.contact), 0);
		if (rcv_contact <= 0)
		{
			perror("rcv error");
			return;
		}
		break;

	case 3: // Update address
		int rcv_address = recv(client_socket, new_customer.address, sizeof(new_customer.address), 0);
		if (rcv_address <= 0)
		{
			perror("rcv error");
			return;
		}
		new_customer.address[rcv_name] = '\0';
		break;

	default:
		break;
	}

	// Open customer database
	int fd = open("customer_db.txt", O_RDWR);
	if (fd < 0)
	{
		perror("error in opening the customer file");
		return;
	}

	// Search for customer record
	struct customer buff;
	while ((read(fd, &buff, sizeof(buff))) > 0)
	{
		if (buff.customer_id == user_id)
		{
			// Handle updates based on menu option
			if (menue_option == 1)
			{
				// Update name with write lock
				struct flock lk;
				lk.l_start = lseek(fd, 0, SEEK_CUR) - sizeof(struct customer);
				lk.l_len = sizeof(struct customer);
				lk.l_type = F_WRLCK;
				lk.l_whence = SEEK_SET;

				if (fcntl(fd, F_SETLKW, &lk) == -1)
				{
					perror("failed to get lock");
					return;
				}

				strcpy(buff.name, new_customer.name);
				if (lseek(fd, -sizeof(struct customer), SEEK_CUR) == -1)
				{
					perror("lseek error");
					close(fd);
					return;
				}

				if (write(fd, &buff, sizeof(buff)) < 0)
				{
					perror("write error");
					close(fd);
					return;
				}

				lk.l_type = F_UNLCK;
				if (fcntl(fd, F_SETLK, &lk) == -1)
				{
					perror("error releasing lock");
					return;
				}

				send(client_socket, "record updated.", strlen("record updated."), 0);
			}
			else if (menue_option == 2)
			{
				// Update contact with write lock
				struct flock lk;
				lk.l_start = lseek(fd, 0, SEEK_CUR) - sizeof(struct customer);
				lk.l_len = sizeof(struct customer);
				lk.l_type = F_WRLCK;
				lk.l_whence = SEEK_SET;

				if (fcntl(fd, F_SETLKW, &lk) == -1)
				{
					perror("failed to get lock");
					return;
				}

				if (lseek(fd, -sizeof(struct customer), SEEK_CUR) == -1)
				{
					perror("lseek error");
					close(fd);
					return;
				}

				buff.contact = new_customer.contact;
				if (write(fd, &buff, sizeof(buff)) < 0)
				{
					perror("write error");
					close(fd);
					return;
				}

				lk.l_type = F_UNLCK;
				if (fcntl(fd, F_SETLK, &lk) == -1)
				{
					perror("error releasing lock");
					return;
				}

				send(client_socket, "record updated.", strlen("record updated."), 0);
			}
			else if (menue_option == 3)
			{
				// Update address with write lock
				struct flock lk;
				lk.l_start = lseek(fd, 0, SEEK_CUR) - sizeof(struct customer);
				lk.l_len = sizeof(struct customer);
				lk.l_type = F_WRLCK;
				lk.l_whence = SEEK_SET;

				if (fcntl(fd, F_SETLKW, &lk) == -1)
				{
					perror("failed to get lock");
					return;
				}

				if (lseek(fd, -sizeof(struct customer), SEEK_CUR) == -1)
				{
					perror("lseek error");
					close(fd);
					return;
				}

				strcpy(buff.address, new_customer.address);
				if (write(fd, &buff, sizeof(buff)) < 0)
				{
					perror("write error");
					close(fd);
					return;
				}

				lk.l_type = F_UNLCK;
				if (fcntl(fd, F_SETLK, &lk) == -1)
				{
					perror("error releasing lock");
					return;
				}

				send(client_socket, "record updated.", strlen("record updated."), 0);
			}
			return;
		}
	}
	send(client_socket, "Record not found", sizeof("Record not found"), 0);
	close(fd);
}

void add_employee(int client_socket)
{
	struct employee new_emp;

	// Receive employee username
	int rcv_name = recv(client_socket, &new_emp.username, sizeof(new_emp.username), 0);
	if (rcv_name <= 0)
	{
		perror("rcv error");
		return;
	}
	new_emp.username[rcv_name] = '\0';

	// Receive employee ID
	int rcv_empid = recv(client_socket, &new_emp.emp_id, sizeof(new_emp.emp_id), 0);
	if (rcv_empid <= 0)
	{
		perror("error in recv");
		close(client_socket);
		return;
	}

	// Set default values
	new_emp.is_online = false;
	new_emp.is_active = true;

	// Handle password
	char temp_pswd[512];
	int rcv_password = recv(client_socket, &temp_pswd, sizeof(temp_pswd), 0);
	if (rcv_password <= 0)
	{
		perror("recv error");
		return;
	}
	temp_pswd[rcv_password] = '\0';

	// Process and hash password
	char hashed_pswd[32];
	char hex_pswd[65];
	trim_trailing_spaces(temp_pswd);
	trim_leading_spaces(temp_pswd);
	hash_password(temp_pswd, hashed_pswd);
	password_hash_to_hex(hashed_pswd, hex_pswd);
	strcpy(new_emp.password, hex_pswd);

	// Check for duplicate employee
	int fd_read = open("employee_db.txt", O_RDWR);
	struct employee temp_emp;
	while ((read(fd_read, &temp_emp, sizeof(struct employee))) > 0)
	{
		if (temp_emp.emp_id == new_emp.emp_id)
		{
			send(client_socket, "employee already exists.", strlen("employee already exists."), 0);
			close(fd_read);
			return;
		}
	}

	// Add new employee to database
	if (lseek(fd_read, 0, SEEK_END) == -1)
	{
		perror("lseek error");
		close(fd_read);
		return;
	}

	if (write(fd_read, &new_emp, sizeof(struct employee)) <= 0)
	{
		perror("write error");
		close(fd_read);
		return;
	}

	send(client_socket, "new employee added successfully", strlen("new employee added successfully"), 0);
	close(fd_read);
}


void edit_employee(int client_socket)
{
	// Receive employee ID to modify
	int emp_id;
	int rcv_emp_id = recv(client_socket, &emp_id, sizeof(emp_id), 0);
	if (rcv_emp_id == -1)
	{
		perror("recv error");
		return;
	}

	// Receive new username
	struct employee new_emp;
	int rcv_empid = recv(client_socket, new_emp.username, sizeof(new_emp.username), 0);
	if (rcv_empid <= 0)
	{
		perror("error in recv");
		close(client_socket);
		return;
	}
	new_emp.username[rcv_empid] = '\0';

	// Update employee record
	int fd_read = open("employee_db.txt", O_RDWR);
	struct employee temp_emp;
	while ((read(fd_read, &temp_emp, sizeof(struct employee))) > 0)
	{
		if (temp_emp.emp_id == emp_id)
		{
			// Apply write lock
			struct flock lk;
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_SET;
			lk.l_start = lseek(fd_read, 0, SEEK_CUR) - sizeof(struct employee);
			lk.l_len = sizeof(struct employee);

			if (fcntl(fd_read, F_SETLKW, &lk) == -1)
			{
				perror("fcntl error");
				close(fd_read);
				return;
			}

			// Update record
			if (lseek(fd_read, -sizeof(struct employee), SEEK_CUR) < 0)
			{
				perror("lseek error");
				close(fd_read);
				return;
			}

			strcpy(temp_emp.username, new_emp.username);
			if (write(fd_read, &temp_emp, sizeof(temp_emp)) < 0)
			{
				perror("write error");
				close(fd_read);
				return;
			}

			// Release lock
			lk.l_type = F_UNLCK;
			if (fcntl(fd_read, F_SETLK, &lk) == -1)
			{
				perror("fcntl error");
				close(fd_read);
				return;
			}

			send(client_socket, "employee updated", strlen("employee updated"), 0);
			close(fd_read);
			return;
		}
	}
	send(client_socket, "employee not updated", strlen("employee not updated"), 0);
	close(fd_read);
}


void deactivate_customer(int customer_id)
{
	int fd_cust = open("customer_db.txt", O_RDWR);
	if (fd_cust == -1)
	{
		perror("open error");
		return;
	}

	struct customer temp_cust;
	while (read(fd_cust, &temp_cust, sizeof(struct customer)) > 0)
	{
		if (temp_cust.customer_id == customer_id)
		{
			// Apply write lock
			struct flock lk;
			lk.l_start = lseek(fd_cust, 0, SEEK_CUR) - sizeof(struct customer);
			lk.l_len = sizeof(struct customer);
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_CUR;

			if (fcntl(fd_cust, F_SETLKW, &lk) == -1)
			{
				perror("failed to get lock");
				return;
			}

			// Deactivate account
			temp_cust.is_active = false;

			// Release lock
			lk.l_type = F_UNLCK;
			if (fcntl(fd_cust, F_SETLK, &lk) == -1)
			{
				perror("failed to release lock");
				return;
			}
			break;
		}
	}
	close(fd_cust);
}


void manage_user_role(int client_socket, int employee_id)
{
	// Open employee database
	int fd_emp = open("employee_db.txt", O_RDWR);
	if (fd_emp == -1)
	{
		perror("open error");
		return;
	}

	struct employee temp_emp;
	// Find and deactivate employee
	while ((read(fd_emp, &temp_emp, sizeof(struct employee))) > 0)
	{
		if (temp_emp.emp_id == employee_id)
		{
			temp_emp.is_active = false;

			// Update employee record
			if (lseek(fd_emp, -sizeof(struct employee), SEEK_CUR) == -1)
			{
				perror("lseek error");
				close(fd_emp);
				return;
			}

			if ((write(fd_emp, &temp_emp, sizeof(struct employee))) <= 0)
			{
				perror("write error");
				close(fd_emp);
				return;
			}
			break;
		}
	}

	// Create new manager record
	struct manager new_manager;
	new_manager.emp_id = temp_emp.emp_id;
	strcpy(new_manager.username, temp_emp.username);
	strcpy(new_manager.password, temp_emp.password);
	new_manager.is_online = false;

	// Open manager database
	int fd_manager = open("manager_db.txt", O_RDWR);
	if (fd_manager == -1)
	{
		perror("open error");
		close(fd_emp);
		return;
	}

	// Apply write lock for adding new manager
	struct flock lk;
	lk.l_type = F_WRLCK;
	lk.l_whence = SEEK_END;
	lk.l_start = 0;
	lk.l_len = sizeof(struct manager);

	if (fcntl(fd_manager, F_SETLKW, &lk) == -1)
	{
		perror("fcntl error");
		close(fd_emp);
		close(fd_manager);
		return;
	}

	// Write new manager record
	if ((write(fd_manager, &new_manager, sizeof(struct manager))) <= 0)
	{
		perror("write error");
		close(fd_emp);
		close(fd_manager);
		return;
	}

	// Release lock
	lk.l_type = F_UNLCK;
	if (fcntl(fd_manager, F_SETLK, &lk) == -1)
	{
		perror("fcntl error");
		close(fd_emp);
		close(fd_manager);
		return;
	}

	// Send success message
	char msg[256];
	sprintf(msg, "Employee '%s' is now manager.", temp_emp.username);
	send(client_socket, msg, strlen(msg), 0);

	// Cleanup
	close(fd_emp);
	close(fd_manager);
}

#endif
