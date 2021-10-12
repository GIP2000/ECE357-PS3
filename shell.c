#include <stdlib.h> 
#include <stdio.h>
#include <string.h>

void cd(char * directory, int len){

}

void pwd(){

}
void exitShell(int code){
    exit(code); 
}

void processLine(char* line, int len){
    if(line[0] == '#') return;
    char * command;
    char * arguments; 
    char * ioRedirect; 

    int commandIndex = strchr(line,' ')-line;
    strncpy(command,line,commandIndex);

    int lessThanIndex = strchr(line,'<') == NULL ? len+1: strchr(line,'<')-line; 
    int greaterThanIndex = strchr(line,'>') == NULL ? len+1: strchr(line,'>')-line;  
    if(greaterThanIndex > 0 && line[greaterThanIndex-1] == '2') greaterThanIndex--; 

    fprintf(stderr,"lessThanIndex: %d\n", lessThanIndex);
    fprintf(stderr,"greaterThanIndex: %d\n", greaterThanIndex); 

    int ioIndex = greaterThanIndex < lessThanIndex ? greaterThanIndex: lessThanIndex;

    fprintf(stderr,"ioIndex: %d\n", ioIndex); 
    
    if(ioIndex != len+1) strncpy(arguments,line+commandIndex+1,ioIndex-(commandIndex+1));
    else strncpy(arguments,line+commandIndex+1,len-(commandIndex+1));
    if(ioIndex != len+1) strncpy(ioRedirect,line+ioIndex,len-ioIndex); 

    fprintf(stderr,"full command: %s\ncommand:%s\nargs:%s\nioRedirect:%s\n",line,command,arguments,ioRedirect); 
    

}

void parseShell(FILE* fp){
    char * line = NULL; 
    size_t len = 0; 
    size_t read; 
    while((read = getline(&line,&len,fp)) != -1)
        processLine(line,read); 
}

int main(int argc, char* argv[]){
    FILE* fp = argc == 1? stdin : fopen(argv[1],"r");
    parseShell(fp); 

}