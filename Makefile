CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lGL -lGLU -lglut  # For graphics (if needed)

# Referee program (main)
REF_SRC = main.c config.c  pipe.c graphics.c
REF_OBJ = $(REF_SRC:.c=.o)

# Player program
PLAYER_SRC = player.c config.c pipe.c
PLAYER_OBJ = $(PLAYER_SRC:.c=.o)

TARGETS = rope_game player

all: $(TARGETS)

rope_game: $(REF_OBJ)
	$(CC) $(REF_OBJ) -o $@ $(LDFLAGS)  # Tab

player: $(PLAYER_OBJ)
	$(CC) $(PLAYER_OBJ) -o $@          # Tab

%.o: %.c constant.h config.h  pipe.h
	$(CC) $(CFLAGS) -c $< -o $@       # Tab

clean:
	rm -f *.o $(TARGETS)
