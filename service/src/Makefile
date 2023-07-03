all: main

main:
	gcc -g -D_XOPEN_SOURCE=500 -DLOG_USE_COLOR -ggdb -Wall -Wextra -o granulizer users.c sharing.c main.c granular.c file_handler.c b64.c/buffer.c b64.c/decode.c b64.c/encode.c log.c/log.c sha256/sha256.c -lm

clean:
	-rm -f granulizer
