
timer: main.c
	gcc -O3 -Wall main.c -o timer

.PHONY: clean
clean:
	rm -fr timer
