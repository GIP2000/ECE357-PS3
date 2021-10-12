all: shell
PHONY: clean

shell: shell.c
	gcc -o shell shell.c

clean: find
	rm -f shell
