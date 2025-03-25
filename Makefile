CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lGL -lGLU -lglut   # hello from ali

SRC = main.c player.c config.c fifo.c pipe.c graphics.c
OBJ = $(SRC:.c=.o)
DEPS = constant.h config.h fifo.h pipe.h

TARGET = rope_game

all: $(TARGET)

$(TARGET): $(OBJ)
    $(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c $(DEPS)
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    rm -f *.o $(TARGET)