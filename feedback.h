#ifndef FEEDBACK_H
#define FEEDBACK_H

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include "commonStruct.h"

void add_feedback(int client_socket, int user_id, char *feedback)
{
    int fd = open("feedback_db.txt", O_RDWR | O_CREAT, 0644);
    if (fd == -1)
    {
        perror("open feedback_db.txt");
        send(client_socket, "Error: Unable to record feedback\n", strlen("Error: Unable to record feedback\n"), 0);
        return;
    }

    struct feedback new_feedback;
    new_feedback.user_id = user_id;
    strncpy(new_feedback.feed_back, feedback, sizeof(new_feedback.feed_back) - 1);
    new_feedback.feed_back[sizeof(new_feedback.feed_back) - 1] = '\0';
    new_feedback.resolved = false;

    struct flock lk = {0};
    lk.l_type = F_WRLCK;
    lk.l_whence = SEEK_END;
    lk.l_len = 0; // Lock till EOF

    if (fcntl(fd, F_SETLKW, &lk) == -1)
    {
        perror("fcntl lock");
        send(client_socket, "Error: Feedback lock failed\n", strlen("Error: Feedback lock failed\n"), 0);
        close(fd);
        return;
    }

    if (write(fd, &new_feedback, sizeof(new_feedback)) != sizeof(new_feedback))
    {
        perror("write feedback");
        send(client_socket, "Error: Writing feedback failed\n", strlen("Error: Writing feedback failed\n"), 0);
    }
    else
    {
        send(client_socket, "Feedback added successfully.\n", strlen("Feedback added successfully.\n"), 0);
    }

    lk.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lk);
    close(fd);
}

/**
 * @brief Manager reviews one unresolved feedback for a given customer
 */
void resolve_feedback(int client_socket, int user_id)
{
    int fd = open("feedback_db.txt", O_RDWR);
    if (fd == -1)
    {
        perror("open feedback_db.txt");
        send(client_socket, "Error: Could not open feedback database\n", strlen("Error: Could not open feedback database\n"), 0);
        return;
    }

    struct feedback temp_feedback;
    bool found = false;
    off_t pos_to_update = -1;

    while (read(fd, &temp_feedback, sizeof(temp_feedback)) == sizeof(temp_feedback))
    {
        if (temp_feedback.user_id == user_id && !temp_feedback.resolved)
        {
            found = true;
            pos_to_update = lseek(fd, -sizeof(temp_feedback), SEEK_CUR);

            // Send feedback to manager
            send(client_socket, temp_feedback.feed_back, strlen(temp_feedback.feed_back), 0);
            return; // send one unresolved feedback at a time
        }
    }

    if (!found)
    {
        send(client_socket, "No unresolved feedback found for this customer.\n", strlen("No unresolved feedback found for this customer.\n"), 0);
    }

    close(fd);
}

/**
 * @brief Update feedback resolution after manager response
 */
void update_feedback_status(int client_socket, int user_id, char response)
{
    int fd = open("feedback_db.txt", O_RDWR);
    if (fd == -1)
    {
        perror("open feedback_db.txt");
        send(client_socket, "Error: Could not update feedback.\n", strlen("Error: Could not update feedback.\n"), 0);
        return;
    }

    struct feedback temp_feedback;
    while (read(fd, &temp_feedback, sizeof(temp_feedback)) == sizeof(temp_feedback))
    {
        if (temp_feedback.user_id == user_id && !temp_feedback.resolved)
        {
            if (response == 'Y' || response == 'y')
            {
                temp_feedback.resolved = true;
                send(client_socket, "Feedback marked as resolved.\n", strlen("Feedback marked as resolved.\n"), 0);
            }
            else
            {
                send(client_socket, "Feedback left unresolved.\n", strlen("Feedback left unresolved.\n"), 0);
            }

            lseek(fd, -sizeof(temp_feedback), SEEK_CUR);
            write(fd, &temp_feedback, sizeof(temp_feedback));
            close(fd);
            return;
        }
    }

    send(client_socket, "No unresolved feedback found for update.\n", strlen("No unresolved feedback found for update.\n"), 0);
    close(fd);
}

#endif // FEEDBACK_H
