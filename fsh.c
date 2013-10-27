#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXIMUM_ARGS 100
#define MAXIMUM_PATH_NAME 100
#define _GNU_SOURCE

char **tokenize(char *input, char *delim, int *counts);
int executeCommands(char **token_arr, bool *exec_flag, bool *exit_flag,
                    bool *exit_param, int counts);

int openReadFile(char *filename, bool* read_file_flag, bool* exec_flag, 
bool *exit_flag, bool *exit_param) {
  FILE *f;  // the file to be printed out
  // try to open the file
  f = fopen(filename, "rb");
  *read_file_flag = false;

  if (f == NULL) {
    return 1;
  }

  // read from the file
  long readbufsize;
  size_t readlen;

  // get the size of the file
  if (fseek(f, 0, SEEK_END) != 0) {
    return 1;
  }

  readbufsize = ftell(f);
  char readbuf[readbufsize];

  if (fseek(f, 0, SEEK_SET) != 0) {
    return 1;
  }
  
  // read from file and write to stdout
  while ((readlen = fread(readbuf, 1, readbufsize, f)) > 0) {}
  readbuf[readbufsize] = '\0';
  // if read failed
  if (ferror(f)) {
    return 1;
  }
  int counts = 0;
  int return_status = 0;
  // remembers if an exec has been called in any of the file lines
  bool exec_called = false;
  char **line_arr = tokenize(readbuf, "\n", &counts);


  for (int i = 0; line_arr[i] != NULL; i++) {
    int counts = 0;
    char **token_arr = tokenize(line_arr[i], " \t", &counts);
    int status = executeCommands(token_arr, exec_flag, 
                    exit_flag, exit_param, counts);

    if (*exec_flag || *exit_flag && status != 0) {
      exec_called = true;
      return_status = status;
    }
    *read_file_flag = true;
    free(token_arr);
    if (*exit_flag) {
      break;
    }
  }
  fclose(f);
  free(line_arr);
  *exec_flag = exec_called;
  return return_status;
}

// splits the given string based on delimiter passed
char** tokenize(char *input, char *delim, int *counts) {
  char *token;
  char *saveptr;
  // allocate an array of all the tokens
  char **token_array = (char**)malloc(sizeof(char*) * MAXIMUM_ARGS);
  int array_size = 0;
  while ((token = strtok_r(input, delim, &saveptr)) != NULL) {
    // populate the array
    token_array[array_size] = token;
    input = NULL;
    array_size++;
  }
  token_array[array_size] = '\0';
  *counts = array_size;
  return token_array;
}

// gets ownership of token_arr
int executeCommands(char **token_arr, bool* exec_flag, bool* exit_flag,
                    bool* exit_param, int counts) {
  int execute_status = 0;
  *exec_flag = false;
  *exit_flag = false;

  char *first_elem = token_arr[0];
  if (strcmp(first_elem, "cd") == 0) {
    // change to directory case
    char *directory = token_arr[1]; // first arg to cd
    int ret;
    ret = chdir(directory);
    if (ret == -1) {
	printf("No such file or directory\n");
        execute_status = 1;
    }
  } else if (strcmp(first_elem, ".") == 0) {
    char *filename = token_arr[1];
    bool read_file_flag = true;
    execute_status = openReadFile(filename, &read_file_flag, 
                 exec_flag, exit_flag, exit_param);
    if (!read_file_flag) {
      printf("File read failed!\n");
    } 
  } else if (strcmp(first_elem, "exit") == 0) {
    if (counts == 2) {
      // there was an optional exit status number
      execute_status = atoi(token_arr[1]);
      *exit_param = true;
    }
    *exit_flag = true;
  } else {
    *exec_flag = true;
    // spawn child process
    int child_pid = fork();
    if (child_pid == 0) {
      // have child attempt to execute file
      if (execvp(first_elem, token_arr) == -1) {
        printf("Command not recognized\n");
        *exit_flag = true;
      }
    } else {
      int status;
      if ((waitpid(child_pid, &status, 0)) == -1) {
        printf("waitpid failed\n");
      }
      execute_status =  WEXITSTATUS(status);
    }
  }
  return execute_status;
}

int main(int argc, char **argv) {
  int exit_status = EXIT_SUCCESS;
  while(1) {
    char *usr_result;

    // format the promt
    const char *end_string = "% ";
    char first[MAXIMUM_PATH_NAME];
    getcwd(first, MAXIMUM_PATH_NAME);
    strncat(first, end_string, 2);

    usr_result = readline(first);
    bool exec_flag = false;
    bool exit_flag = false;
    // checks whether exit had a parameter
    bool exit_param = false;
    int execute_status = 0;
    if (strcmp(usr_result, "") != 0) {
      int counts = 0;
      char **token_arr = tokenize(usr_result, " \t", &counts);
      execute_status = executeCommands(token_arr, &exec_flag, &exit_flag,
                                       &exit_param, counts);
      free(token_arr);
    }
    free(usr_result);

    // Check whether or not an exec call was made, and if so,
    // update the exit status
    if (exec_flag || exit_param) {
      exit_status = execute_status;
    }

    if (exit_flag) {
      exit(exit_status);
    }
  }
} 
