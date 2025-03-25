#ifndef PIPE_H
#define PIPE_H

// For each player, we have a pipe for child -> parent communication
// fds[i][0] is read end, fds[i][1] is write end
int init_pipes(int fds[][2], int n);

// read from child => returns number of bytes read
int read_effort(int read_fd, void *buffer, int buf_size);

// write from child => returns number of bytes written
int write_effort(int write_fd, void *buffer, int buf_size);

#endif
