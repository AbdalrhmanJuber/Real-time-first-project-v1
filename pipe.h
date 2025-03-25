// pipe.h
#ifndef PIPE_H
#define PIPE_H

#define MAX_PLAYERS 8

// A pair of fds per player: fds[i][0] = read, fds[i][1] = write
int init_pipes(int fds[MAX_PLAYERS][2]);
int close_pipe_ends(int fds[2], int read, int write);
int send_to_parent(int write_fd, int value);
int read_from_child(int read_fd);

#endif
