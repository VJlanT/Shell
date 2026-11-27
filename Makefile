CC      = gcc
CFLAGS  = -Wall -Wextra -Werror
SRC     = $(wildcard src/*.c)

mysh: $(SRC) src/shell.h
	$(CC) $(CFLAGS) $(SRC) -o $@

clean:
	rm -f mysh

.PHONY: clean
