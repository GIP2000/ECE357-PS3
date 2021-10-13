#include <stdlib.h>
#include <stdio.h>


//print the first argument to stderr if there is no argument print Hello World
int main(int argc, char *argv[]){
    if(argc == 1)
        fprintf(stderr, "Hello World\n");
    else
        fprintf(stderr, "%s\n", argv[1]);
    if(argc == 3)
        printf("%s\n", argv[2]);
    return 0;
}

