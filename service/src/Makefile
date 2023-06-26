all: main

main:
	gcc -g -D_XOPEN_SOURCE=500 -DLOG_USE_COLOR -ggdb -Wall -Wextra -o granulizer users.c sharing.c main.c granular.c file_handler.c base64.c log.c/log.c sha256/sha256.c -lm -lreadline

clean:
	-rm granulizer
