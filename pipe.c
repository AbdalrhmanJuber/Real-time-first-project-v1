#include <stdio.h>
#include <unistd.h>
#include "pipe.h"

int init_pipes(int fds[][2], int n)
{
    for (int i = 0; i < n; i++) {
        if (pipe(fds[i]) == -1) {
            perror("pipe creation failed");
            return -1;
        }
    }
    return 0;
}

int read_effort(int read_fd, void *buffer, int buf_size)
{
    return read(read_fd, buffer, buf_size);
}

int write_effort(int write_fd, void *buffer, int buf_size)
{
    return write(write_fd, buffer, buf_size);
}
