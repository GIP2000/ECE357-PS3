all: shell
PHONY: clean

shell: shell.c
	gcc -o shell shell.c

clean: shell
	rm -f shell
