#include <stdlib.h> 
#include <stdio.h>
#include <stdbool.h> 
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

void processIORedirect(char * ioRedirect){
    fprintf(stderr,"ioRedirect: %s; len:%d; firstchar:%d\n",ioRedirect,strlen(ioRedirect),(int)ioRedirect[0]);  
    char * token = strtok(ioRedirect," ");
    do{
        int source = -1; 
        char * desName; 
        int flag = 0; 
        int mode = 0666; 
        switch(token[0]){
            case '2': 
                // redirect stderr
                source = STDERR_FILENO; 
                strcpy(token,token+1); // i didn't break so I can continue and just use the code from the next section cause its the same 
                // flag = token[2] == '>' ? O_APPEND|O_CREAT|O_WRONLY : O_TRUNC|O_CREAT|O_WRONLY;
                // int offset = token[2] == '>' ?3:2; 
                // desName = malloc(strlne(token) - offset+1);
                // strcpy(desName,token+offset);
                // desName[strlne(token) - offset] = '\0'; 
                // break; 
            case '>':
                //redirect stdout
                source = source == -1 ? STDOUT_FILENO: source; 
                flag = token[1] == '>' ? O_APPEND|O_CREAT|O_WRONLY : O_TRUNC|O_CREAT|O_WRONLY;
                int offset = token[1] == '>' ?2:1; 
                desName = malloc(strlen(token) - offset+1);
                strcpy(desName,token+offset);
                desName[strlen(token) - offset] = '\0';
                break;
            case '<':
                // redirect stdin
                source = STDIN_FILENO; 
                desName = malloc(strlen(token)); 
                strcpy(desName,token+1);
                flag = O_RDONLY; 
                desName[strlen(token)] = '\0'; 
                break;
            default: fprintf(stderr,"unknown error"); exit(-1);  
        }
        int fd = open(desName,flag,mode); 
        if(fd == -1){
            fprintf(stderr,"Error opening file name \"%s\":%s\n",desName,strerror(errno)); 
            exit(-1); 
        }
        dup2(fd,source); 
        free(desName); 
    }while((token = strtok(NULL," ")) != NULL); 

}
void execute(char * command, char * arguments){
    int currentSize = sizeof(char*)*(strlen(command)+1); 
    char ** argv = (char**)malloc(currentSize); 
    argv[0] = command;
    int i = 1;   
    char * token = strtok(arguments," ");
    do{
        currentSize+=sizeof(char*)*(strlen(token)) ;
        argv = (char**)realloc(argv,currentSize);
        argv[i++] = token; 
    }while((token = strtok(NULL," ")) != NULL);
    argv[i] = NULL;
    execvp(command,argv); 
    perror("execvp"); 
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
void external(char* command, char* arguments, char* ioRedirect, bool hasRedirect){
    int pid; 
    switch((pid = fork())){
        case -1: perror("Error forking: "); exit(-1);
        case 0: {// child
            if(hasRedirect) processIORedirect(ioRedirect);
            execute(command,arguments);
            break;
        }
        default: {// parent
            int cpid;
            struct rusage ru;
            int status;
            if((cpid == wait3(&status,0,&ru)) == -1){
                perror("Error waiting for child: ");
                exit(-1);
            } else {
                printf("Child process %d consumed%ld.%.6d seconds of user time \n",
                pid,
                ru.ru_utime.tv_sec,
                ru.ru_utime.tv_usec);
            }
        }

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
    else external(command,arguments,ioRedirect,ioIndex != len+1); 

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