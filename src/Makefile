all: clean main

main:
	gcc -Wall -Wextra -o main libaudiosynth/lib/portsf.c libaudiosynth/lib/ieee80.c -lm src/*.c

clean:
	-rm main
