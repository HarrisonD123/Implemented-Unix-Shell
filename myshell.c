#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>


enum redirection {REDIR, ADVREDIR};
typedef enum redirection redirection;

/*My shell's own error message*/
void error_message() {
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

/*Built-in Command Functions*/
void call_exit(char** myargv) {

    if(myargv[1] != NULL) {
        error_message();
    }
    else {
        exit(0);
    }
}

void call_cd(char** myargv) {
    int success;
    if(myargv[1] == NULL) {
        chdir(getenv("HOME"));
    }
    else {
        if(myargv[2] != NULL) {
            error_message();
            return;
        }

        success = chdir(myargv[1]);
        if(success < 0) {
            error_message();
        }
    }
}

void call_pwd(char** myargv) {
    char cwd[500];

    if(myargv[1] != NULL) {
        error_message();
    }
    else {
        getcwd(cwd, 500);
        cwd[strlen(cwd) + 1] = '\0';
        cwd[strlen(cwd)] = '\n';
        write(STDOUT_FILENO, cwd, strlen(cwd));
    }
}

/*END OF BUILT_IN COMMANDS*/

/*Helper for printing messages using the write() system call*/
void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

/*If the input is too long, read the rest of it and ignore it*/
void handle_too_long(char* pinput, FILE *fp){
    while(1) {
        pinput = fgets(pinput, 514, fp);
        if (!pinput) {
            break;
        }
        myPrint(pinput);
        if (pinput[strlen(pinput)-1] == '\n') {
            break;
        }
        
    }
    error_message();
    
}

/*Test whether the input is all whitespace for error checking*/
int all_white_space(char* pinput) {
    while(*pinput != '\0') {
        if(!isspace(*pinput)) {
            return 0;
        }
        pinput++;
    }
    return 1;
}

/*Parse input if multiple commands are entered in one line, separated by
  ;'s. Place commands into a string array*/
void parse_cmds(char* cmd_buff, char** multcmds) {
    cmd_buff[strlen(cmd_buff) - 1] = '\0';
    char* token = strtok(cmd_buff, ";");
    int i = 0;

    while(token != NULL) {
        multcmds[i] = token;
        i++;
        token = strtok(NULL, ";");
    }
    multcmds[i] = NULL;
}

/*Parse the arguments in a command and place them in a string array*/
void parse_args(char* buff, char** myargv) {
    char* token = strtok(buff, " \t");
    int i = 0;

    while(token != NULL) {
        myargv[i] = token;
        i++;
        token = strtok(NULL, " \t");
    }
    myargv[i] = NULL;
}

/*Test whether there are more than one redirection symbols in a command.
  This will throw an error in the calling function*/
int too_many_redir(char* cmd) {
    int num_redir = 0;
    while(*cmd != '\0') {
        if(*cmd == '>') {
            num_redir += 1;
        }
        cmd++;
    }
    return num_redir - 1;
}

/*Separate the arguments from the filename in a redirection command*/
int parse_redir(char* buff, char** parsed_redir, redirection mode) {
    char* delim;

    if(mode == REDIR) {
        delim = ">";
    }
    else {
        delim = ">+";
    }

    char* token = strtok(buff, delim);
    int i = 0;

    while(token != NULL) {
        if(i > 1) {
            return 0;
        }
        parsed_redir[i] = token;
        i++;
        token = strtok(NULL, delim);
    }
    if(i < 2) {
        return 0;
    }
    else {
        return 1;
    }
}

/*Creates a child process to execute a simple command*/
void exec_cmd(char** myargv) {
    if(fork() == 0) {
        if(execvp(myargv[0], myargv) < 0) {
            error_message();
            exit(0);
        }
    }
    else {
        wait(NULL);
    }
}

/*Executes a redirection command. Creates a new file and uses dup2 to
  redirect execvp's output from the screen to the file's file descriptor*/
void exec_redir_cmd(char** myargv, char* filename) {
    int fd;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    fd = open(filename, O_RDWR | O_CREAT, mode);

    if(fd < 0) {
        error_message();
        return;
    }
    if(fork() == 0) {    
        dup2(fd, STDOUT_FILENO);
        if(execvp(myargv[0], myargv) < 0) {
            error_message();
            exit(0);
        }
    }
    else {
        wait(NULL);
        close(fd);
    }
}

/*Executes an advanced redirection command. Adds the command's output to the
  front of an existing file by: 1) copying the file to a new temp file,
  2) overwriting the file with the command's output, and
  3) reading the temp file's contents and writing them back to the end of the
  original file*/
void exec_advredir_cmd(char** myargv, char* filename) {
    char buf[100];
    int bytes_read;
    int fda;
    int fdb;
    int fdc;
    int fd_tmp;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    fda = open(filename, O_RDONLY, mode);
    fd_tmp = open("temp", O_RDWR | O_CREAT | O_TRUNC, mode);

    if(fda < 0 || fd_tmp < 0) {
        error_message();
        return;
    }

    while((bytes_read = read(fda, buf, 99)) > 0) {
        buf[bytes_read] = '\0';
        write(fd_tmp, buf, strlen(buf));
    }
    lseek(fd_tmp, 0, SEEK_SET);

    if(fork() == 0) {   
        fdc =  open(filename, O_WRONLY | O_TRUNC, mode);
        if(fdc < 0) {
            error_message();
            return;
        }
        dup2(fdc, STDOUT_FILENO);
        if(execvp(myargv[0], myargv) < 0) {
            error_message();
            exit(0);
        }
    }
    else {
        wait(NULL);
        fdb = open(filename, O_WRONLY | O_APPEND, mode);
        if(fdb < 0) {
            error_message();
            return;
        }
        while((bytes_read = read(fd_tmp, buf, 99)) > 0) {
            buf[bytes_read] = '\0';
            write(fdb, buf, strlen(buf));
        }
        close(fda);
        close(fdb);
        close(fd_tmp);
    }
}

/*Calls functions to handle input for a simple command*/
void simple_command(char* cmd, char** myargv) {
    parse_args(cmd, myargv);

    if(myargv[0] == NULL) {
        return;
    }
    else if(strcmp(myargv[0], "exit") == 0) {
        call_exit(myargv);
    }
    else if(strcmp(myargv[0], "cd") == 0) {
        call_cd(myargv);
    }
    else if(strcmp(myargv[0], "pwd") == 0) {
        call_pwd(myargv);
    }

    else {
        exec_cmd(myargv);
    }
}

/*Error tests, then calls functions for either redirection or advanced
  redirection, depending on the value of 'mode'*/
void redir_command(char* cmd, char** myargv, redirection mode) {
    char* parsed_redir[2];
    char* filename[20];
    
    if (too_many_redir(cmd)) {
        error_message();
        return;
    }

    else if(parse_redir(cmd, parsed_redir, mode) == 0) {
        error_message();
        return;
    }

    else if((all_white_space(parsed_redir[0])) ||
            (all_white_space(parsed_redir[1]))) {
        error_message();
        return;
    }

    parse_args(parsed_redir[0], myargv);
    parse_args(parsed_redir[1], filename);

    if(filename[1] != NULL) {
        error_message();
        return;
    } 
    else if(myargv[0] == NULL) {
        return;
    }
    else if(strcmp(myargv[0], "exit") == 0) {//cannot redirect exit, cd, pwd
        error_message();
        return;
    }
    else if(strcmp(myargv[0], "cd") == 0) {
        error_message();
        return;
    }
    else if(strcmp(myargv[0], "pwd") == 0) {
        error_message();
        return;
    }

    else {
        if(access(filename[0], F_OK) == 0) {
            if(mode == REDIR) {
                error_message();
                return;
            }
            else {
                exec_advredir_cmd(myargv, filename[0]);
            }      
        }
        else {
            exec_redir_cmd(myargv, filename[0]);
        }
    }
}

int main(int argc, char *argv[]) 
{
    char cmd_buff[514];
    char *pinput;
    char* multcmds[50];
    char* myargv[100];
    FILE *fp;

    if(argc > 2) {
        error_message();
        exit(0);
    }
    if(argc == 2) { //If program is run with a batch file, Batch Mode
        if((fp = fopen(argv[1], "r")) == NULL) {
            error_message();
            exit(0);
        }
        /*read from batch file*/
        while((pinput = fgets(cmd_buff, 514, fp))) {
            if(all_white_space(pinput)) {
                continue;
            }
            myPrint(cmd_buff);
        
            if ((strlen(pinput) >= 513) && (pinput[strlen(pinput)-1] != '\n')) {
                handle_too_long(pinput, fp);
                continue;
            }
    
            /*Determine whether the command is simple, a redirection, or an
              advanced redirection*/
            else {
                parse_cmds(cmd_buff, multcmds);
                int i = 0;
                while(multcmds[i] != NULL) {
                    if(strstr(multcmds[i], ">+")) {
                        redir_command(multcmds[i], myargv, ADVREDIR);
                    }
                    else if(strstr(multcmds[i], ">")) {
                        redir_command(multcmds[i], myargv, REDIR);
                    }
                    else {
                        simple_command(multcmds[i], myargv);
                    }
                    i++;
                }
            }
        }
    }

    else {//If no batch file, run in Interactive Mode
        while (1) {
        myPrint("myshell> ");
        /*read from command line*/
        pinput = fgets(cmd_buff, 514, stdin); //if 514 == '\0' and 513 != \n -> 513 chars read -> throw away, throw error, and read rest and throw away
        if (!pinput) {
            error_message();
            exit(0);
        }
        myPrint(cmd_buff);
        
        if ((strlen(pinput) >= 513) && (pinput[strlen(pinput)-1] != '\n')) {
            handle_too_long(pinput, stdin);
            continue;
        }
        

        else {
            parse_cmds(cmd_buff, multcmds);
            int i = 0;
            while(multcmds[i] != NULL) {
                if(strstr(multcmds[i], ">+")) {
                    redir_command(multcmds[i], myargv, ADVREDIR);
                }
                else if(strstr(multcmds[i], ">")) {
                    redir_command(multcmds[i], myargv, REDIR);
                }
                else {
                    simple_command(multcmds[i], myargv);
                }
                i++;
            }
        }
    }
    }
}