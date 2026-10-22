#include <stdlib.h>
#include "common.h"

static void check_line(const char *line, size_t length)
{
    if (length > 0 && (line[length - 1] == '.' || line[length - 1] == ';')) {
        write_all(1, line, length);
        write_all(1, "\n", 1);
        return;
    }
    write_all(2, "Invalid line (must end with . or ;): ", 37);
    write_all(2, line, length);
    write_all(2, "\n", 1);
}

int main(void)
{
    char buffer[4096];
    char *line = NULL;
    size_t length = 0;
    size_t capacity = 0;

    while (1) {
        ssize_t count = read(0, buffer, sizeof(buffer));
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            fail("read failed");
        }
        if (count == 0) {
            break;
        }
        for (ssize_t i = 0; i < count; i++) {
            if (buffer[i] == '\n') {
                check_line(line, length);
                length = 0;
                continue;
            }
            if (length + 1 > capacity) {
                capacity = capacity ? capacity * 2 : 128;
                char *grown = realloc(line, capacity);
                if (grown == NULL) {
                    fail("realloc failed");
                }
                line = grown;
            }
            line[length++] = buffer[i];
        }
    }

    if (length > 0) {
        check_line(line, length);
    }
    free(line);
    return 0;
}
