all: shell
PHONY: clean

shell: shell.c
	gcc -o shell shell.c

clean: shell
	rm -f shell

test.out: test.c
	gcc -o test.out test.c

debug.out: shell.c
	gcc -o debug.out shell.c -g
