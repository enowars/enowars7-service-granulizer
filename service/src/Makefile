all: main

main:
	gcc -g -D_XOPEN_SOURCE=500 -ggdb -Wall -Wextra -o granulizer users.c main.c granular.c tinywav.c file_handler.c base64.c

clean:
	-rm granulizer
