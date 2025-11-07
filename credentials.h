#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <ctype.h>
#include <errno.h>
#include "commonStruct.h"

/* ---------- Utility functions ---------- */

void trim_trailing_spaces(char *str)
{
	if (str == NULL)
		return;
	int len = strlen(str);
	while (len > 0 && isspace((unsigned char)str[len - 1]))
		len--;
	str[len] = '\0';
}

void trim_leading_spaces(char *str)
{
	if (str == NULL)
		return;
	// find first non-space
	char *p = str;
	while (*p && isspace((unsigned char)*p))
		p++;
	if (p != str)
		memmove(str, p, strlen(p) + 1); // +1 to move terminating '\0'
}

/* Hash using EVP-SHA256 */
void hash_password(const char *pswd, unsigned char *hashed_pswd)
{
	if (pswd == NULL || hashed_pswd == NULL)
		return;
	EVP_MD_CTX *context = EVP_MD_CTX_new();
	if (!context)
		return;
	const EVP_MD *md = EVP_sha256();
	EVP_DigestInit_ex(context, md, NULL);
	EVP_DigestUpdate(context, pswd, strlen(pswd));
	EVP_DigestFinal_ex(context, hashed_pswd, NULL);
	EVP_MD_CTX_free(context);
}

// password_hash_to_hex(hashed_pswd, hex_pswd);
void password_hash_to_hex(const unsigned char *hashed_pswd, char *hex_pswd)
{
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		sprintf(hex_pswd + (i * 2), "%02x", hashed_pswd[i]);
	hex_pswd[SHA256_DIGEST_LENGTH * 2] = '\0'; // FIXED: added null terminator
}

/* ---------- Helper: safe open with create ---------- */
static int safe_open_rw_create(const char *path)
{
	int fd = open(path, O_RDWR | O_CREAT, 0644);
	if (fd < 0)
	{
		perror(path);
	}
	return fd;
}

/* ---------- Password edit functions (customer/employee/manager/admin) ---------- */
void edit_credentials_customer(int client_socket, int user_id)
{
	struct customer temp;
	char temp_pswd[256];
	int fd = safe_open_rw_create("customer_db.txt");
	if (fd < 0)
		return;

	int rec_client = recv(client_socket, temp_pswd, sizeof(temp_pswd) - 1, 0);
	if (rec_client <= 0)
	{
		perror("recv error");
		close(fd);
		return;
	}
	temp_pswd[rec_client] = '\0';
	trim_trailing_spaces(temp_pswd);
	trim_leading_spaces(temp_pswd);

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.customer_id == user_id)
		{
			// take write lock for the record
			struct flock lk = {0};
			off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_SET;
			lk.l_start = rec_offset;
			lk.l_len = sizeof(temp);
			if (fcntl(fd, F_SETLKW, &lk) == -1)
			{
				perror("locking error");
				close(fd);
				return;
			}

			unsigned char hashed_pswd[SHA256_DIGEST_LENGTH];
			char hex_pswd[(SHA256_DIGEST_LENGTH * 2) + 1];
			hash_password(temp_pswd, hashed_pswd);
			password_hash_to_hex(hashed_pswd, hex_pswd);
			strncpy(temp.password, hex_pswd, sizeof(temp.password) - 1);
			temp.password[sizeof(temp.password) - 1] = '\0';

			if (lseek(fd, rec_offset, SEEK_SET) == -1)
			{
				perror("lseek error");
				// release lock
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}
			if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
			{
				perror("write error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}

			// release lock
			lk.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &lk) == -1)
				perror("unlock error");

			send(client_socket, "Password updated successfully", strlen("Password updated successfully"), 0);
			close(fd);
			return;
		}
	}
	send(client_socket, "User credentials not present in db.", strlen("User credentials not present in db."), 0);
	close(fd);
}

void edit_credentials_employee(int client_socket, int user_id)
{
	struct employee temp;
	char temp_pswd[256];
	int fd = safe_open_rw_create("employee_db.txt");
	if (fd < 0)
		return;

	int rec_client = recv(client_socket, temp_pswd, sizeof(temp_pswd) - 1, 0);
	if (rec_client <= 0)
	{
		perror("recv error");
		close(fd);
		return;
	}
	temp_pswd[rec_client] = '\0';
	trim_trailing_spaces(temp_pswd);
	trim_leading_spaces(temp_pswd);

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.emp_id == user_id)
		{
			struct flock lk = {0};
			off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_SET;
			lk.l_start = rec_offset;
			lk.l_len = sizeof(temp);
			if (fcntl(fd, F_SETLKW, &lk) == -1)
			{
				perror("locking error");
				close(fd);
				return;
			}

			unsigned char hashed_pswd[SHA256_DIGEST_LENGTH];
			char hex_pswd[(SHA256_DIGEST_LENGTH * 2) + 1];
			hash_password(temp_pswd, hashed_pswd);
			password_hash_to_hex(hashed_pswd, hex_pswd);
			strncpy(temp.password, hex_pswd, sizeof(temp.password) - 1);
			temp.password[sizeof(temp.password) - 1] = '\0';

			if (lseek(fd, rec_offset, SEEK_SET) == -1)
			{
				perror("lseek error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}
			if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
			{
				perror("write error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}

			lk.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &lk) == -1)
				perror("unlock error");

			send(client_socket, "Password updated successfully", strlen("Password updated successfully"), 0);
			close(fd);
			return;
		}
	}
	send(client_socket, "User credentials not present in db.", strlen("User credentials not present in db."), 0);
	close(fd);
}

void edit_credentials_manager(int client_socket, int user_id)
{
	struct manager temp;
	char temp_pswd[256];
	int fd = safe_open_rw_create("manager_db.txt");
	if (fd < 0)
		return;

	int rec_client = recv(client_socket, temp_pswd, sizeof(temp_pswd) - 1, 0);
	if (rec_client <= 0)
	{
		perror("recv error");
		close(fd);
		return;
	}
	temp_pswd[rec_client] = '\0';
	trim_trailing_spaces(temp_pswd);
	trim_leading_spaces(temp_pswd);

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.emp_id == user_id)
		{
			struct flock lk = {0};
			off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_SET;
			lk.l_start = rec_offset;
			lk.l_len = sizeof(temp);
			if (fcntl(fd, F_SETLKW, &lk) == -1)
			{
				perror("locking error");
				close(fd);
				return;
			}

			unsigned char hashed_pswd[SHA256_DIGEST_LENGTH];
			char hex_pswd[(SHA256_DIGEST_LENGTH * 2) + 1];
			hash_password(temp_pswd, hashed_pswd);
			password_hash_to_hex(hashed_pswd, hex_pswd);
			strncpy(temp.password, hex_pswd, sizeof(temp.password) - 1);
			temp.password[sizeof(temp.password) - 1] = '\0';

			if (lseek(fd, rec_offset, SEEK_SET) == -1)
			{
				perror("lseek error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}
			if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
			{
				perror("write error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}

			lk.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &lk) == -1)
				perror("unlock error");

			send(client_socket, "Password updated successfully", strlen("Password updated successfully"), 0);
			close(fd);
			return;
		}
	}
	send(client_socket, "User credentials not present in db.", strlen("User credentials not present in db."), 0);
	close(fd);
}

void edit_credentials_admin(int client_socket, int user_id)
{
	struct admin temp;
	char temp_pswd[256];
	int fd = safe_open_rw_create("admin_db.txt");
	if (fd < 0)
		return;

	int rec_client = recv(client_socket, temp_pswd, sizeof(temp_pswd) - 1, 0);
	if (rec_client <= 0)
	{
		perror("recv error");
		close(fd);
		return;
	}
	temp_pswd[rec_client] = '\0';
	trim_trailing_spaces(temp_pswd);
	trim_leading_spaces(temp_pswd);

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.emp_id == user_id)
		{
			struct flock lk = {0};
			off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_SET;
			lk.l_start = rec_offset;
			lk.l_len = sizeof(temp);
			if (fcntl(fd, F_SETLKW, &lk) == -1)
			{
				perror("locking error");
				close(fd);
				return;
			}

			unsigned char hashed_pswd[SHA256_DIGEST_LENGTH];
			char hex_pswd[(SHA256_DIGEST_LENGTH * 2) + 1];
			hash_password(temp_pswd, hashed_pswd);
			password_hash_to_hex(hashed_pswd, hex_pswd);
			strncpy(temp.password, hex_pswd, sizeof(temp.password) - 1);
			temp.password[sizeof(temp.password) - 1] = '\0';

			if (lseek(fd, rec_offset, SEEK_SET) == -1)
			{
				perror("lseek error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}
			if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
			{
				perror("write error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}

			lk.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &lk) == -1)
				perror("unlock error");

			send(client_socket, "Password updated successfully", strlen("Password updated successfully"), 0);
			close(fd);
			return;
		}
	}
	send(client_socket, "User credentials not present in db.", strlen("User credentials not present in db."), 0);
	close(fd);
}

/* ---------- Authentication functions ---------- */
/* These compute the hex hash and compare. If they update is_online they take a write lock briefly. */

bool authenticate_customer(int client_socket, int user_id, char *pswd)
{
	struct customer temp;
	unsigned char hashed_pswd[SHA256_DIGEST_LENGTH];
	char hex_pswd[(SHA256_DIGEST_LENGTH * 2) + 1];

	if (!pswd)
		return false;
	trim_trailing_spaces(pswd);
	trim_leading_spaces(pswd);
	hash_password(pswd, hashed_pswd);
	password_hash_to_hex(hashed_pswd, hex_pswd);

	int fd = safe_open_rw_create("customer_db.txt");
	if (fd < 0)
		return false;

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.customer_id == user_id)
		{
			if (!temp.is_active)
			{
				close(fd);
				return false;
			}
			if (temp.is_online)
			{
				close(fd);
				return false;
			}

			if (strncmp(temp.password, hex_pswd, sizeof(temp.password)) == 0)
			{
				// update online status with a lock
				struct flock lk = {0};
				off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
				lk.l_type = F_WRLCK;
				lk.l_whence = SEEK_SET;
				lk.l_start = rec_offset;
				lk.l_len = sizeof(temp);
				if (fcntl(fd, F_SETLKW, &lk) == -1)
				{
					perror("locking error");
					close(fd);
					return false;
				}

				temp.is_online = true;
				if (lseek(fd, rec_offset, SEEK_SET) == -1)
				{
					perror("lseek error");
					lk.l_type = F_UNLCK;
					fcntl(fd, F_SETLK, &lk);
					close(fd);
					return false;
				}
				if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
				{
					perror("write error");
					lk.l_type = F_UNLCK;
					fcntl(fd, F_SETLK, &lk);
					close(fd);
					return false;
				}

				lk.l_type = F_UNLCK;
				if (fcntl(fd, F_SETLK, &lk) == -1)
					perror("unlock error");
				close(fd);
				return true;
			}
		}
	}
	close(fd);
	return false;
}

bool authenticate_employee(int client_socket, int user_id, char *pswd)
{
	struct employee temp;
	unsigned char hashed_pswd[SHA256_DIGEST_LENGTH];
	char hex_pswd[(SHA256_DIGEST_LENGTH * 2) + 1];

	if (!pswd)
		return false;
	trim_trailing_spaces(pswd);
	trim_leading_spaces(pswd);
	hash_password(pswd, hashed_pswd);
	password_hash_to_hex(hashed_pswd, hex_pswd);

	int fd = safe_open_rw_create("employee_db.txt");
	if (fd < 0)
		return false;

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.emp_id == user_id)
		{
			if (temp.is_online)
			{
				close(fd);
				return false;
			}
			if (!temp.is_active)
			{
				close(fd);
				return false;
			}

			if (strncmp(temp.password, hex_pswd, sizeof(temp.password)) == 0)
			{
				struct flock lk = {0};
				off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
				lk.l_type = F_WRLCK;
				lk.l_whence = SEEK_SET;
				lk.l_start = rec_offset;
				lk.l_len = sizeof(temp);
				if (fcntl(fd, F_SETLKW, &lk) == -1)
				{
					perror("locking error");
					close(fd);
					return false;
				}

				temp.is_online = true;
				if (lseek(fd, rec_offset, SEEK_SET) == -1)
				{
					perror("lseek error");
					lk.l_type = F_UNLCK;
					fcntl(fd, F_SETLK, &lk);
					close(fd);
					return false;
				}
				if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
				{
					perror("write error");
					lk.l_type = F_UNLCK;
					fcntl(fd, F_SETLK, &lk);
					close(fd);
					return false;
				}

				lk.l_type = F_UNLCK;
				if (fcntl(fd, F_SETLK, &lk) == -1)
					perror("unlock error");
				close(fd);
				return true;
			}
		}
	}
	close(fd);
	return false;
}

bool authenticate_manager(int client_socket, int user_id, char *pswd)
{
	struct manager temp;
	unsigned char hashed_pswd[SHA256_DIGEST_LENGTH];
	char hex_pswd[(SHA256_DIGEST_LENGTH * 2) + 1];

	if (!pswd)
		return false;
	trim_trailing_spaces(pswd);
	trim_leading_spaces(pswd);
	hash_password(pswd, hashed_pswd);
	password_hash_to_hex(hashed_pswd, hex_pswd);

	int fd = safe_open_rw_create("manager_db.txt");
	if (fd < 0)
		return false;

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.emp_id == user_id)
		{
			if (temp.is_online)
			{
				close(fd);
				return false;
			}

			if (strncmp(temp.password, hex_pswd, sizeof(temp.password)) == 0)
			{
				struct flock lk = {0};
				off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
				lk.l_type = F_WRLCK;
				lk.l_whence = SEEK_SET;
				lk.l_start = rec_offset;
				lk.l_len = sizeof(temp);
				if (fcntl(fd, F_SETLKW, &lk) == -1)
				{
					perror("locking error");
					close(fd);
					return false;
				}

				temp.is_online = true;
				if (lseek(fd, rec_offset, SEEK_SET) == -1)
				{
					perror("lseek error");
					lk.l_type = F_UNLCK;
					fcntl(fd, F_SETLK, &lk);
					close(fd);
					return false;
				}
				if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
				{
					perror("write error");
					lk.l_type = F_UNLCK;
					fcntl(fd, F_SETLK, &lk);
					close(fd);
					return false;
				}

				lk.l_type = F_UNLCK;
				if (fcntl(fd, F_SETLK, &lk) == -1)
					perror("unlock error");
				close(fd);
				return true;
			}
		}
	}
	close(fd);
	return false;
}

bool authenticate_admin(int client_socket, int user_id, char *pswd)
{
	struct admin temp;
	unsigned char hashed_pswd[SHA256_DIGEST_LENGTH];
	char hex_pswd[(SHA256_DIGEST_LENGTH * 2) + 1];

	if (!pswd)
		return false;
	trim_trailing_spaces(pswd);
	trim_leading_spaces(pswd);
	hash_password(pswd, hashed_pswd);
	password_hash_to_hex(hashed_pswd, hex_pswd);

	int fd = safe_open_rw_create("admin_db.txt");
	if (fd < 0)
		return false;

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.emp_id == user_id)
		{
			if (temp.is_online)
			{
				close(fd);
				return false;
			}

			if (strncmp(temp.password, hex_pswd, sizeof(temp.password)) == 0)
			{
				struct flock lk = {0};
				off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
				lk.l_type = F_WRLCK;
				lk.l_whence = SEEK_SET;
				lk.l_start = rec_offset;
				lk.l_len = sizeof(temp);
				if (fcntl(fd, F_SETLKW, &lk) == -1)
				{
					perror("locking error");
					close(fd);
					return false;
				}

				temp.is_online = true;
				if (lseek(fd, rec_offset, SEEK_SET) == -1)
				{
					perror("lseek error");
					lk.l_type = F_UNLCK;
					fcntl(fd, F_SETLK, &lk);
					close(fd);
					return false;
				}
				if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
				{
					perror("write error");
					lk.l_type = F_UNLCK;
					fcntl(fd, F_SETLK, &lk);
					close(fd);
					return false;
				}

				lk.l_type = F_UNLCK;
				if (fcntl(fd, F_SETLK, &lk) == -1)
					perror("unlock error");
				close(fd);
				return true;
			}
		}
	}
	close(fd);
	return false;
}

/* ---------- Logout functions (set is_online = false) ---------- */
/* These also take a simple write lock while updating the record */

void customer_logout(int client_socket, int user_id)
{
	struct customer temp;
	int fd = safe_open_rw_create("customer_db.txt");
	if (fd < 0)
		return;

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.customer_id == user_id)
		{
			struct flock lk = {0};
			off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_SET;
			lk.l_start = rec_offset;
			lk.l_len = sizeof(temp);
			if (fcntl(fd, F_SETLKW, &lk) == -1)
			{
				perror("locking error");
				close(fd);
				return;
			}

			temp.is_online = false;
			if (lseek(fd, rec_offset, SEEK_SET) == -1)
			{
				perror("lseek error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}
			if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
			{
				perror("write error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}

			lk.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &lk) == -1)
				perror("unlock error");

			send(client_socket, "Logged out.\n", strlen("Logged out.\n"), 0);
			close(fd);
			return;
		}
	}
	send(client_socket, "Could not logout", strlen("Could not logout"), 0);
	close(fd);
}

void employee_logout(int client_socket, int user_id)
{
	struct employee temp;
	int fd = safe_open_rw_create("employee_db.txt");
	if (fd < 0)
		return;

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.emp_id == user_id)
		{
			struct flock lk = {0};
			off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_SET;
			lk.l_start = rec_offset;
			lk.l_len = sizeof(temp);
			if (fcntl(fd, F_SETLKW, &lk) == -1)
			{
				perror("locking error");
				close(fd);
				return;
			}

			temp.is_online = false;
			if (lseek(fd, rec_offset, SEEK_SET) == -1)
			{
				perror("lseek error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}
			if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
			{
				perror("write error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}

			lk.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &lk) == -1)
				perror("unlock error");

			send(client_socket, "Logged out.\n", strlen("Logged out.\n"), 0);
			close(fd);
			return;
		}
	}
	send(client_socket, "Could not logout", strlen("Could not logout"), 0);
	close(fd);
}

void manager_logout(int client_socket, int user_id)
{
	struct manager temp;
	int fd = safe_open_rw_create("manager_db.txt");
	if (fd < 0)
		return;

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.emp_id == user_id)
		{
			struct flock lk = {0};
			off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_SET;
			lk.l_start = rec_offset;
			lk.l_len = sizeof(temp);
			if (fcntl(fd, F_SETLKW, &lk) == -1)
			{
				perror("locking error");
				close(fd);
				return;
			}

			temp.is_online = false;
			if (lseek(fd, rec_offset, SEEK_SET) == -1)
			{
				perror("lseek error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}
			if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
			{
				perror("write error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}

			lk.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &lk) == -1)
				perror("unlock error");

			send(client_socket, "Logged out.\n", strlen("Logged out.\n"), 0);
			close(fd);
			return;
		}
	}
	send(client_socket, "Could not logout", strlen("Could not logout"), 0);
	close(fd);
}

void admin_logout(int client_socket, int user_id)
{
	struct admin temp;
	int fd = safe_open_rw_create("admin_db.txt");
	if (fd < 0)
		return;

	lseek(fd, 0, SEEK_SET);
	while (read(fd, &temp, sizeof(temp)) == sizeof(temp))
	{
		if (temp.emp_id == user_id)
		{
			struct flock lk = {0};
			off_t rec_offset = lseek(fd, -((off_t)sizeof(temp)), SEEK_CUR);
			lk.l_type = F_WRLCK;
			lk.l_whence = SEEK_SET;
			lk.l_start = rec_offset;
			lk.l_len = sizeof(temp);
			if (fcntl(fd, F_SETLKW, &lk) == -1)
			{
				perror("locking error");
				close(fd);
				return;
			}

			temp.is_online = false;
			if (lseek(fd, rec_offset, SEEK_SET) == -1)
			{
				perror("lseek error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}
			if (write(fd, &temp, sizeof(temp)) != sizeof(temp))
			{
				perror("write error");
				lk.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lk);
				close(fd);
				return;
			}

			lk.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &lk) == -1)
				perror("unlock error");

			send(client_socket, "Logged out.\n", strlen("Logged out.\n"), 0);
			close(fd);
			return;
		}
	}
	send(client_socket, "Could not logout", strlen("Could not logout"), 0);
	close(fd);
}

#endif
