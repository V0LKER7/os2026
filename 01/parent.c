#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include "common.h"

static size_t read_file_name(char *file_name, size_t capacity) {
    size_t length = 0;
    char symbol;

    while (1) {
        ssize_t count = read(0, &symbol, 1);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            fail("read file name failed");
        }
        if (count == 0 || symbol == '\n') {
            break;
        }
        if (length + 1 >= capacity) {
            errno = ENAMETOOLONG;
            fail("file name");
        }
        file_name[length++] = symbol;
    }
    file_name[length] = '\0';
    return length;
}

int main(void)
{
    char file_name[256];
    char buffer[4096];
    int pipe1[2];
    int pipe2[2];

    write_all(1, "Enter file name: ", 17);
    if (read_file_name(file_name, sizeof(file_name)) == 0) {
        errno = EINVAL;
        fail("file name");
    }

    int file_fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0) {
        fail("open failed");
    }
    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        fail("pipe failed");
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        fail("fork failed");
    }

    if (child_pid == 0) {
        if (dup2(pipe1[0], 0) < 0 || dup2(file_fd, 1) < 0 || dup2(pipe2[1], 2) < 0) {
            fail("dup2 failed");
        }
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        close(file_fd);
        execl("./child", "child", (char *)NULL);
        fail("exec failed");
    }

    close(pipe1[0]);
    close(pipe2[1]);
    close(file_fd);

    struct pollfd watched[2];
    watched[0].fd = 0;
    watched[0].events = POLLIN;
    watched[1].fd = pipe2[0];
    watched[1].events = POLLIN;

    while (1) {
        if (poll(watched, 2, -1) < 0) {
            if (errno == EINTR) {
                continue;
            }
            fail("poll failed");
        }

        if (watched[0].revents & (POLLIN | POLLHUP)) {
            ssize_t count = read(0, buffer, sizeof(buffer));
            if (count < 0) {
                if (errno == EINTR) {
                    continue;
                }
                fail("read stdin failed");
            }
            if (count == 0) {
                close(pipe1[1]);
                watched[0].fd = -1;
            } else {
                write_all(pipe1[1], buffer, count);
            }
        }

        if (watched[1].revents & (POLLIN | POLLHUP)) {
            ssize_t count = read(pipe2[0], buffer, sizeof(buffer));
            if (count < 0) {
                if (errno == EINTR) {
                    continue;
                }
                fail("read pipe2 failed");
            }
            if (count == 0) {
                break;
            }
            write_all(1, buffer, count);
        }
    }

    close(pipe2[0]);
    if (waitpid(child_pid, NULL, 0) < 0) {
        fail("waitpid failed");
    }
    return 0;
}
