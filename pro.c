#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "credentials.h"
#include <openssl/evp.h>
#include <openssl/sha.h>

int main()
{
	int fd = open("admin_db.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		perror("open admin_db.txt");
		return 1;
	}

	struct admin temp;
	memset(&temp, 0, sizeof(temp));

	temp.emp_id = 1;
	strncpy(temp.username, "admin1", sizeof(temp.username) - 1);
	temp.username[sizeof(temp.username) - 1] = '\0';

	char psd[128] = "pswd";
	unsigned char hashed_pswd[SHA256_DIGEST_LENGTH];
	char hex_pswd[(SHA256_DIGEST_LENGTH * 2) + 1];

	hash_password(psd, hashed_pswd);
	password_hash_to_hex(hashed_pswd, hex_pswd);

	strncpy(temp.password, hex_pswd, sizeof(temp.password) - 1);
	temp.password[sizeof(temp.password) - 1] = '\0';

	printf("hex password: %s\n", temp.password);

	temp.is_online = false;

	ssize_t w = write(fd, &temp, sizeof(struct admin));
	if (w != sizeof(struct admin))
	{
		perror("write error");
		close(fd);
		return 1;
	}

	close(fd);
	return 0;
}
