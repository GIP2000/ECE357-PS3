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

int prevExitCode = 0; 

// performes the IO redirection using the dup2 sys call 
void processIORedirect(char * ioRedirect){
    char * token = strtok(ioRedirect," ");
    do{
        int source = -1; 
        char * desName; 
        int flag = 0; 
        const int mode = 0666; 
        switch(token[0]){
            case '2': 
                // redirect stderr
                source = STDERR_FILENO; 
                strcpy(token,token+1); // fall through
            case '>':
                //redirect stdout or stderr if fall through
                source = source == -1 ? STDOUT_FILENO: source; 
                flag = token[1] == '>' ? O_APPEND|O_CREAT|O_WRONLY : O_TRUNC|O_CREAT|O_WRONLY;
                int offset = token[1] == '>' ?2:1; 
                desName = (char*)malloc(sizeof(char)*(strlen(token) - offset+1));
                strcpy(desName,token+offset);
                desName[strlen(token) - offset] = '\0';
                break;
            case '<':
                // redirect stdin
                source = STDIN_FILENO; 
                desName = (char*)malloc(sizeof(char)*(strlen(token))); 
                strcpy(desName,token+1);
                flag = O_RDONLY; 
                desName[strlen(token)] = '\0'; 
                break;
            default: fprintf(stderr,"Unknown symbol when performing IO Redirection\n"); exit(-1);  
        }
        int fd = open(desName,flag,mode); 
        if(fd == -1){
            fprintf(stderr,"Error opening file name \"%s\" when redirecting IO. Error:%s\n",desName,strerror(errno)); 
            exit(errno); 
        }
        if(dup2(fd,source) == -1){
            fprintf(stderr,"Unknown symbol when performing IO Redirection\n"); 
            exit(errno); 
        };
        close(fd);  
        free(desName); 
    }while((token = strtok(NULL," ")) != NULL); 

}

// parses all the arguments and executes the program via the execvp sys call 
void execute(char * command, char * arguments){
    size_t currentSize = sizeof(char)*(strlen(command)+1); 
    char ** argv = (char**)malloc(currentSize); 
    if(argv == NULL) {
        perror("Error Allocating Memorey: ");
        exit(errno);
    }
    argv[0] = command;
    int i = 1;   
    char * token = strtok(arguments," ");
    while(token != NULL){
        currentSize+=sizeof(char)*(strlen(token)) ;
        if((argv = (char**)realloc(argv,currentSize)) == NULL){
            perror("Error Allocating Memorey: ");
            exit(errno);
        }
        argv[i++] = token; 
        token = strtok(NULL," "); 
    }
    argv[i] = NULL;
    execvp(command,argv); 
    perror("execvp");
    exit(127);  
}


// performes the pwd command internally
void pwd(){
    int size = PATH_MAX == -1 || PATH_MAX > 10240 ? 10240:PATH_MAX;
    char * buf; 
    char * ptr; 
    for(buf = ptr = NULL; ptr == NULL; size*=2){
        if((buf = realloc(buf,size)) == NULL){perror("Error allocating Mem: ");break;} 
        ptr = getcwd(buf,size); 
        if(ptr == NULL && errno != ERANGE) {prevExitCode = errno;perror("Error getting Directory: ");break;} 
    }
    if(buf != NULL)printf("%s\n",buf); 

}

//performes the cd command internally 
void cd(char * directory, int len){
    char * dir;
    if(directory == NULL || strlen(directory) == 0) dir = getenv("HOME");
    else dir = strtok(directory, " ");
    if(chdir(dir) != 0) {
        perror("Error Changing Directory: "); 
        prevExitCode = errno;
    } 
}

// runs the exit command internally
void exitShell(char* arguments){
    char * code = strtok(arguments," ");
    code = code == NULL ? arguments:code; 

    if(strlen(code) <= 0) exit(prevExitCode); 

    //checks if the string is a digit
    int i; 
    for(i = 1; i<strlen(code); i++)
        if(isdigit(code[i]) == 0 || (i == 0 && code[i] == '-')) {
        fprintf(stderr, "Couldn't exit error code not an integer\n");
        return;
    } 
    exit(atoi(code));
}

// runs an external command by forking the process and executing as well as waiting (in the parent) with the wait3 sys call 
void external(char* command, char* arguments, char* ioRedirect, bool hasRedirect){
    int pid; 
    struct timeval stop,start; 
    gettimeofday(&start,NULL); 
    switch((pid = fork())){
        case -1: perror("Error forking: "); exit(errno);
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
                exit(errno);
            }

            // calculates the real time using the gettimeofday sys call 
            gettimeofday(&stop,NULL); 
            suseconds_t real_u = 1000000 * (stop.tv_sec - start.tv_sec) + stop.tv_usec - start.tv_usec; 
            time_t real_s = real_u / 1000000; 
            real_u %= 1000000; 
            if(status != 0 ){
                if (WIFSIGNALED(status)){
                    prevExitCode = WTERMSIG(status);
                    printf("Child process %d exited with signal %d\n", pid,WTERMSIG(status));}
                else {
                    prevExitCode = WEXITSTATUS(status);
                    printf("Child process %d exited with return value %d\n", pid, WEXITSTATUS(status));}
            } else {
                prevExitCode = 0;
                printf("Child process %d exited normally\n",pid); 
            }
            printf("Real:%ld.%.6ds User:%ld.%.6ds Sys:%ld.%.6ds\n",
            real_s,
            real_u,
            ru.ru_utime.tv_sec,
            ru.ru_utime.tv_usec,
            ru.ru_stime.tv_sec,
            ru.ru_stime.tv_usec);
            
        }

    }
}

// processes an input line and parses the different types of arguments into seperate strings 
void processLine(char* line, int len){
    if(line[0] == '#' || len == 1) return;
    char* command;
    char* arguments; 
    char* ioRedirect; 

    // gets the command 
    int commandIndex = strchr(line,' ') == NULL ? len-1:strchr(line,' ')-line ;
    command = (char *)malloc(commandIndex+1); 
    strncpy(command,line,commandIndex);
    command[commandIndex] = '\0';

    // find the place where the args stop and the the io redirect starts
    int lessThanIndex = strchr(line,'<') == NULL ? len+1: strchr(line,'<')-line; 
    int greaterThanIndex = strchr(line,'>') == NULL ? len+1: strchr(line,'>')-line;  
    if(greaterThanIndex < len && line[greaterThanIndex-1] == '2') greaterThanIndex--; 
    int ioIndex = greaterThanIndex < lessThanIndex ? greaterThanIndex: lessThanIndex;
    
    //seperates the args and the io redirect (if present )
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

    // determnes if I should use an inbuilt command or not 
    if(strcmp(command,"cd") == 0) cd(arguments,len-(commandIndex+1));
    else if(strcmp(command,"pwd") == 0) pwd(); 
    else if(strcmp(command,"exit") == 0) exitShell(arguments); 
    else external(command,arguments,ioRedirect,ioIndex != len+1); 

    free(command);     
    if(ioIndex != len+1) {free(ioRedirect); free(arguments);}
    else if(len-(commandIndex+1) != -1) free(arguments); 
}

// reads the input file line by line 
void parseShell(FILE* fp){
    char * line = NULL; 
    size_t len = 0; 
    size_t read; 
    while((read = getline(&line,&len,fp)) != -1){
        processLine(line,read); 
    }
}

int main(int argc, char* argv[]){
    FILE* fp = argc == 1? stdin : fopen(argv[1],"r");
    if(fp == NULL) {
        perror("Error opening script file: "); 
        exit(errno); 
    }
    parseShell(fp); 
    return prevExitCode;
}