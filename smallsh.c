#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>

// adapted from https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"



int main()
{
    // CREATE A PLACE TO STORE USER INPUT
    //
    // adapted from "How to declare an array of strings in C" https://www.youtube.com/watch?v=ly66E_LGubk
    // initializing string to store user input with length 2048
    char user_input[2048];
    // initializing array of pointers
    char* args_array[512];
    // initializing a variable to point to an element in the args array
    int count = 0;
    char command [100];
    char* input_file[1];
    bool input_flag = false;
    char* output_file[1];
    bool output_flag = false;
    bool background_mode = false;
    int pid;
    char pid_str[7];
    int status = 0;
    pid_t background_pids_array[500];
    memset(background_pids_array, '\0', sizeof(background_pids_array));
    int background_pids_count = 0;
    int background_status = 0;
    

    // start looping and continue until command = exit
    do {
      // CLEAN UP VARIABLES
      //
      // memset usage adapted from https://stackoverflow.com/questions/17288859/using-memset-for-integer-array-in-c
      memset(user_input, '\0', sizeof(user_input));
      memset(args_array, '\0', sizeof(args_array));
      memset(command, '\0', sizeof(command));
      count = 0;
      memset(input_file, '\0', sizeof(input_file));
      input_flag = false;
      memset(output_file, '\0', sizeof(output_file));
      output_flag = false;
      background_mode = false;
      pid = getpid();
      memset(pid_str, '\0', sizeof(pid_str));
      // convert int to str adapted from https://stackoverflow.com/questions/41051931/function-opposite-to-atoi
      sprintf(pid_str,"%d", pid);
      fflush(stdout);

      // CHECK WHETHER ANY BACKGROUND PROCESSES HAVE COMPLETED
      //
      // loop through em til you reach a '\0'
      for (int i = 0; i < background_pids_count; i++) {
        if (background_pids_array[i] != '\0') {
          // adapted from https://canvas.oregonstate.edu/courses/1890465/pages/exploration-process-api-monitoring-child-processes?module_item_id=22511469
          // WNOHANG specified. If the child hasn't terminated, waitpid will immediately return with value 0
          if (waitpid(background_pids_array[i], &background_status, WNOHANG) != 0) {
            if (WIFEXITED(background_status)){
              printf("Background pid %d is done: exit value %d\n", background_pids_array[i], background_status);
              fflush(stdout);
            } else { 
              printf("Background pid %d is done: terminated by signal %d\n", background_pids_array[i], WTERMSIG(background_status));
              fflush(stdout);
            }
            background_pids_array[i] = '\0';
          }

        }
      }

  
      // GET USER INPUT
      //
      // adapted from "How to read a line of text in C" https://www.youtube.com/watch?v=Lksi1HEMZgY
      // requesting user input
      printf(RED "w" YELLOW "h" GREEN "a" CYAN "t" BLUE "'" MAGENTA "s " RED "u" YELLOW "p" GREEN "?" CYAN "? " BLUE ": ");
      fflush(stdout);
      // storing user input to user_input for 2048 chars to read from stdin
      fgets(user_input, 2048, stdin);
      // replacing newline at end of input with null terminator
      user_input[strlen(user_input) - 1] = '\0'; 
      // printf("You said: %s%s", user_input, "\n"); // validating able to get user input
      printf("\n");
      // $$ EXPANSION 
      //
      // learned abt strstr from https://www.tutorialspoint.com/c_standard_library/c_function_strstr.htm
      // adapted looping strstr from https://cboard.cprogramming.com/c-programming/73365-how-use-strstr-find-all-occurrences-substring-string-not-only-first.html 
      char replacement_str[2048];
      memset(replacement_str, '\0', sizeof(replacement_str));
      // copy the base user_input string in to the replacement_str
      strcat(replacement_str, user_input);
      // create pointer to what we're looking through
      char *pointer = user_input;
      // track replacements made so far so we can terminate replacement string w/necessary shift given PID is longer than "$$"
      int replacements_made = 0;
      // while the substring exists, find $$
      while ((pointer = strstr(pointer, "$$")) != NULL) {
        // adapted from What is size_t in C? https://www.youtube.com/watch?v=gWOeL1oymrc 
        size_t start = pointer - user_input;
        size_t end = start + 2;
        //printf("%lu%lu", start, end); // validating able to get $$ locations
        //fflush(stdout);
        // if we've already replaced some "$$", where something is in the user_input string is NOT where it is in the replacement string
        // because the PID replacing "$$" was longer. extra_space finds out how much longer
        int extra_space = (strlen(pid_str) - 2) * replacements_made;
        // truncate the replacement string at the starting pointer ("$$") by making that value the null terminator
        replacement_str[start + extra_space] = '\0';
        // append the PID
        strcat(replacement_str, pid_str);
        // append the rest of the user_input string
        strcat(replacement_str, user_input + end);
        // increment replacements made and move the pointer forward
        replacements_made += 1;
        pointer += 2;
        //printf("replacement str: %s\nuser input: %s\n", replacement_str, user_input); // validating able to replace $$
      }
      

      // SPLIT USER INPUT INTO PIECES
      //
      // adapted from "How to split strings in C (strtok)" https://www.youtube.com/watch?v=34DnZ2ewyZo
      // string token -- split the input string
      char* piece = strtok(replacement_str, " \t\r\n"); // delimiter suggestion from discord
      // only parse lines that don't start with comments
      // because we're now checking the first char of piece, we need to account for empty input here
      if (piece && piece[0] != '#') {
        while (piece != NULL) {
          // if the user indicates that the next arg will be either an input or output file, set flag to indicate
          if (strcmp(piece, "<") == 0) { input_flag = true; }
          else if (strcmp(piece, ">") == 0) { output_flag = true; }
          // if the flag is set, assign the piece as input or output file & clear flag
          else if (input_flag) {
            input_file[0] = piece;
            input_flag = false;
          }
          else if (output_flag) {
            output_file[0] = piece;
            output_flag = false;
          }
          // otherwise it's either normal or the ending &, which we'll check for outside of the loop
          else {
            // store input values, broken into pieces, in array
            args_array[count] = piece;
            count += 1;
          }
          // continuing to split the string
          piece = strtok(NULL, " \t\r\n");
        }
        // if the last argument is an ampersand, set background mode to true
        if (strcmp(args_array[count - 1], "&") == 0) {
          background_mode = true;
          // replace that arg with null so it is no longer treated as an arg
          args_array[count - 1] = NULL;
        }

      

        // BUILT-IN COMMANDS
        //
        //
        if (strcmp(args_array[0], "cd") == 0) {
          int chdir_error;
          // if count is 1, the only thing passed was a command
          if (count == 1) { chdir_error = chdir(getenv("HOME")); } 
          // otherwise, second arg is the path/file
          else { chdir_error = chdir(args_array[1]); }
          // print an error, if one exists
          if (chdir_error == -1) { 
            perror("chdir"); }
        
          // validating updating working directory
          //char CWD[256];
          //getcwd(CWD, sizeof(CWD));
          //printf("%s\n", CWD);
          //fflush(stdout);

        } else if (strcmp(args_array[0], "exit") == 0) {
          strcat(command, "exit");
          // kill any child processes/jobs
          for (int i = 0; i < background_pids_count; i++) {
            if (background_pids_array[i] != '\0') {
              kill(background_pids_array[i], SIGTERM);
            }
          }
          // kms
          exit(0);
        } else if (strcmp(args_array[0], "status") == 0) {
          // prints out either the exit status or the terminating signal of the last foreground process ran by your shell (excluding built-ins)
          // TODO: terminating signal
          printf("exit value %d\n", status);
          fflush(stdout);
        } else {
          // NON-BUILT IN COMMANDS
          //
          // from https://canvas.oregonstate.edu/courses/1890465/pages/exploration-process-api-executing-a-new-program?module_item_id=22511470
          int childStatus;
          int sourceFD;
          int targetFD;

	        // Fork a new process
	        pid_t spawnPid = fork();

	        switch(spawnPid){
	        case -1:
		        perror("fork()\n");
            status = 1;
		        exit(1);
		        break;
	        case 0:
		        // In the child process
            // INPUT/OUTPUT REDIRECTION
            // from https://canvas.oregonstate.edu/courses/1890465/pages/exploration-processes-and-i-slash-o?module_item_id=22511479
          
            // INPUT
            // if there is an input file or we're in background mode, redirection is needed
            if (input_file[0] || background_mode) {
              // if there is an input file, this takes precedence
              if (input_file[0]) { sourceFD = open(input_file[0], O_RDONLY); }
              // if there is not an input file, then we are in background mode -- redirect to /dev/null
              else { sourceFD = open("/dev/null", O_RDONLY); }
              // if opening either file resulted in an error, report it, update status, and exit
	            if (sourceFD == -1) { 
		            perror("source open()"); 
                status = 1;
		            exit(1); 
	            }
              // Redirect stdin to source file
	            int result = dup2(sourceFD, 0);
	            if (result == -1) { 
		            perror("source dup2()"); 
                status = 1;
		            exit(2);
              }}
           
           
            // OUTPUT
            // if there is an output file or we're in background mode, redirection is needed
            if (output_file[0] || background_mode) {
              // if there is an output file, this takes precedence
              if (output_file[0]) { targetFD = open(output_file[0], O_WRONLY | O_CREAT | O_TRUNC, 0644); }
              // if there is not an output file, then we are in background mode -- redirect to /dev/null
              else { targetFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644); }
              // if opening either file resulted in an error, report it, update status, and exit
  	          if (targetFD == -1) { 
		            perror("target open()"); 
                status = 1;
		            exit(1); 
	            }
              // Redirect stdout to target file
	            int result = dup2(targetFD, 1);
	            if (result == -1) { 
		            perror("target dup2()"); 
                status = 1;
		            exit(2); 
	            }}

            //printf("CHILD(%d) running command\n", getpid());
		        execvp(args_array[0], args_array);
		        // exec only returns if there is an error
		        perror(args_array[0]);
            status = 1;
		        exit(2);
		        break;

	        default:
		        // In the parent process
            if (background_mode) {
              // in background mode, the shell will print the pid of a background process when it begins
              printf("Background pid is %d\n", spawnPid);
              fflush(stdout);
              // add pid to background pids array
              background_pids_array[background_pids_count] = spawnPid;
              background_pids_count += 1;
              // the shell must not wait for such a command to complete
              // the parent must return command line access and control to the user immediately after forking off the child
            } else {
              // in foreground mode, we wait for child's termination
		          spawnPid = waitpid(spawnPid, &childStatus, 0);
              // if a child process has completed, update status
              if (childStatus != 0) { status = 1; } 
              else { status = 0; }
		          //printf("PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnPid);
            }
            printf("\n");
            printf(RED "h" YELLOW "o" GREEN "p" CYAN "e " BLUE "t" MAGENTA "h" RED "a" YELLOW "t " GREEN "h" CYAN "e" BLUE "l" MAGENTA "p" RED "e" YELLOW "d" GREEN "." CYAN "." BLUE "." MAGENTA "." RED "." YELLOW "." GREEN "." CYAN "." BLUE "." RESET"\n");
            printf("\n");
            fflush(stdout);

		        break;
          } 
        }
      }
    } while (strcmp(command, "exit") != 0 ); // stop looping once command = exit
    return 0;
}
