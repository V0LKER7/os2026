#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <stddef.h>
#include <unistd.h>

#define chunk_size 4096

static inline size_t str_len(const char *text) {
    size_t length = 0;
    while (text[length] != '\0') {
        length++;
    }
    return length;
}

static inline int write_all(int fd, const char *data, size_t length) {
    size_t written_total = 0;
    while (written_total < length) {
        ssize_t written = write(fd, data + written_total, length - written_total);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        written_total += (size_t)written;
    }
    return 0;
}

static inline void fail(const char *message) {
    write_all(2, message, str_len(message));
    write_all(2, "\n", 1);
    _exit(1);
}

#endif
