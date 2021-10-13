#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>

void processIORedirect(char * ioRedirect){
    char * token = strtok(ioRedirect," ");
    do{
        switch(ioRedirect[0]){
            case '2': break; 
            case '>':break;
            case '<':break;
            default: fprintf(stderr,"unknown error"); exit(-1);  
        }
    }while((token = strtok(NULL," ")) != NULL); 

}
void execute(char * command, char * arguments){
    char * token = strtok(arguments," ");
    do{

    }while((token = strtok(NULL," ")) != NULL); 
}

void pwd(){
    int size = PATH_MAX == -1 || PATH_MAX > 10240 ? 10240:PATH_MAX;
    char * buf; 
    char * ptr; 
    for(buf = ptr = NULL; ptr == NULL; size*=2){
        if((buf = realloc(buf,size)) == NULL){perror("Error allocating Mem: ");break;} 
        ptr = getcwd(buf,size); 
        if(ptr == NULL && errno != ERANGE) {perror("Error getting Directory: ");break;} 
    }
    if(buf != NULL)printf("%s\n",buf); 

}
void cd(char * directory, int len){
    char * dir;
    if(directory == NULL || strlen(directory) == 0) dir = getenv("HOME");
    else dir = strtok(directory, " ");
    if(chdir(dir) != 0) perror("Error Changing Directory: "); 
}
void exitShell(char* code){
    // change to parse for the code; 
    exit(atoi(code));
}
void external(char* command, char* arguments, char* ioRedirect){
    int pid; 
    switch((pid = fork())){
        case -1: perror("Error forking: "); exit(-1);
        case 0: 
            processIORedirect(ioRedirect);
            execute(command,arguments);
            break;
        default:
            //wait; 
    }
}
void processLine(char* line, int len){
    if(line[0] == '#') return;
    char* command;
    char* arguments; 
    char* ioRedirect; 

    int commandIndex = strchr(line,' ') == NULL ? len-1:strchr(line,' ')-line ;
    command = (char *)malloc(commandIndex+1); 
    strncpy(command,line,commandIndex);
    command[commandIndex] = '\0';

    int lessThanIndex = strchr(line,'<') == NULL ? len+1: strchr(line,'<')-line; 
    int greaterThanIndex = strchr(line,'>') == NULL ? len+1: strchr(line,'>')-line;  
    if(greaterThanIndex > 0 && line[greaterThanIndex-1] == '2') greaterThanIndex--; 
    int ioIndex = greaterThanIndex < lessThanIndex ? greaterThanIndex: lessThanIndex;
    
    if(ioIndex != len+1) {
        arguments = malloc(ioIndex-(commandIndex+1)+1); 
        strncpy(arguments,line+commandIndex+1,ioIndex-(commandIndex+1));
        arguments[ioIndex-(commandIndex+1)] = '\0';
        ioRedirect = (char *)malloc(len-ioIndex);
        strcpy(ioRedirect,line+ioIndex);
        ioRedirect[len-ioIndex-1] = '\0';
    } else if(len-(commandIndex+1) != -1){
        arguments = (char *)malloc(len-(commandIndex+1)); 
        strcpy(arguments,line+commandIndex+1);
        arguments[len-(commandIndex+1)-1] = '\0';
    }

    if(strcmp(command,"cd") == 0) cd(arguments,len-(commandIndex+1));
    else if(strcmp(command,"pwd") == 0) pwd(); 
    else if(strcmp(command,"exit") == 0) exitShell(arguments); 
    else external(command,arguments,ioRedirect); 

    free(command);     
    if(ioIndex != len+1) {free(ioRedirect); free(arguments);}
    else if(len-(commandIndex+1) != -1) free(arguments); 
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