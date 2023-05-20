all: main

main:
	gcc -g -D_XOPEN_SOURCE=500 -DLOG_USE_COLOR -ggdb -Wall -Wextra -o test_wave test_wave_reader.c ../src/users.c ../src/granular.c ../src/tinywav.c ../src/file_handler.c ../src/base64.c ../src/log.c/log.c

clean:
	-rm test_wave
