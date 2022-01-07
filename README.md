# Fully functional shell written in C using systems calls, concurrency, and file descriptors. #

Task
---
Implement a simplified shell that can execute commands like a Unix shell and print the output to the screen.  
  
#### Provide support for: ####
1. simple commands
2. multiple simple commands on one line, separated by semicolons `ls;cal 1 2022`
3. support excess white space or semicolons between commands `ls    ;;;; ;;; ; cal 1 2022`
4. support redirection `ls > file.txt`
5. support advanced redirection that inserts a command's output at the start of an existing file `ls >+ file.txt`
6. support batch files, where multiple lines of commands can be read from a file

How to run locally
---
- Run this command `git clone https://github.com/HarrisonD123/Implemented-Unix-Shell.git`
- In the the project directory, run this command in the terminal: `make myshell`
- To run the program, enter in the terminal: `./myshell`
- Once the program is running, type any shell command to see the output
- To redirect output to a file type `<command> > <filename>`. Make sure the file name does not already exist in the directory
- To use advanced redirection, type `<command> >+ <filename>`
- There are batch files in the p4shell/batch-files directory. To run the shell with a batch file, type `./myshell` followed by the
  batchfile directory: `./myshell batch-files/1KCommand`
- Use `exit` to exit the program

NOTE
---
Only myshell.c is part of my project. Everything else was preprepared. All code is in C.