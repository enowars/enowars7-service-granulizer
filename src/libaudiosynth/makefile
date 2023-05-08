make:
	gcc lib/portsf.c lib/ieee80.c *.c -lm -g -o demo_libaudiosynth
	#-lm links math library
start:
	make
	./demo_libaudiosynth
clean:
	rm demo_libaudiosynth
	rm output.wav