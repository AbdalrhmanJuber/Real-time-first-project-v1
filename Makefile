CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lGL -lGLU -lglut -lm

# Separate executables
TARGETS = rope_game player graphics

all: $(TARGETS)

# Main game (no graphics code)
rope_game: main.o config.o pipe.o
	$(CC) $^ -o $@

# Graphics visualization
graphics: graphics.o
	$(CC) $^ -o $@ $(LDFLAGS)

# Player process
player: player.o config.o pipe.o
	$(CC) $^ -o $@

%.o: %.c constant.h config.h pipe.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)