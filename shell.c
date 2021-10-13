#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>

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
    if(directory == NULL) dir = "~/"; 
    else dir = strtok(directory, " ");
    fprintf(stderr,"dir: %s\n",dir);
    if(chdir(dir) != 0) perror("Error Changing Directory: "); 
    else pwd(); 
}

void exitShell(char* code){
    // change to parse for the code; 
    exit(atoi(code));
}
void external(char* command, char* arguments, char* ioRedirect){

}
void processLine(char* line, int len){
    if(line[0] == '#') return;
    char * command;
    char * arguments; 
    char * ioRedirect; 

    int commandIndex = strchr(line,' ') == NULL ? len-1:strchr(line,' ')-line ;
    fprintf(stderr,"commandIndex:%d\nLetters:\n",commandIndex);
    command = malloc(commandIndex); 
    strncpy(command,line,commandIndex);
    int i;
    for(i = 0;i<strlen(command); i++){
        fprintf(stderr,"%d:%d\n",i,(int)command[i]); 
    }

    int lessThanIndex = strchr(line,'<') == NULL ? len+1: strchr(line,'<')-line; 
    int greaterThanIndex = strchr(line,'>') == NULL ? len+1: strchr(line,'>')-line;  
    if(greaterThanIndex > 0 && line[greaterThanIndex-1] == '2') greaterThanIndex--; 
    int ioIndex = greaterThanIndex < lessThanIndex ? greaterThanIndex: lessThanIndex;
    
    if(ioIndex != len+1) {
        arguments = malloc(ioIndex-(commandIndex+1)); 
        strncpy(arguments,line+commandIndex+1,ioIndex-(commandIndex+1));
        ioRedirect = malloc(len-ioIndex);
        strcpy(ioRedirect,line+ioIndex);
    } else if(len-(commandIndex+1) != -1){
        arguments = malloc(len-(commandIndex+1)); 
        strcpy(arguments,line+commandIndex+1);
    }

    fprintf(stderr,"full command:%s:%d\ncommand:%s:%d\nargs:%s:%d\n",line,strlen(line),command,strlen(command),arguments,strlen(arguments)); 

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