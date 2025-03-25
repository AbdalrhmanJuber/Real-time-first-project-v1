// fifo.h
#ifndef FIFO_H
#define FIFO_H

#define FIFO_DIR "/tmp/"
#define FIFO_PREFIX "team_"

// Each team could have its own FIFO file: /tmp/team_1, /tmp/team_2

int create_fifo(const char *name);
int write_fifo(const char *name, const char *msg);
int read_fifo(const char *name, char *buffer, int bufsize);
void remove_fifo(const char *name);

#endif
