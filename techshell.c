// Name: Aayusha Khadka
// CSC 222 Techshell project
// Description: A simple shell that displays a prompt, read user
// input, parses commands and arguements , supports cd, run other
// commands using fork/execvp, and supports < and > redirection

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_INPUT_LEN 1024
#define MAX_ARGS 128

// Struct to store parsed command details
struct ShellCommand {
  char *command;
  char **arguments; // NULL terminated array of arguments
  int numArgs;
  char *inputFile;  // File for standard input redirection (or NULL)
  char *outputFile; // File for standard output redirection (or NULL)
};

// Function prototypes
void displayPrompt();
char *getInput();
struct ShellCommand parseInput(char *input);
void executeCommand(struct ShellCommand cmd);
void freeCommand(struct ShellCommand *cmd);

int main() {
  char *input;
  struct ShellCommand command;

  // Repeadtely prompt the user for input
  for (;;) {
    // Display the prompt
    displayPrompt();

    // Get the user's input
    input = getInput();
    if (input == NULL) {
      // EOF (Ctrl+D)
      printf("\n");
      break;
    }

    // Parse the command line
    command = parseInput(input);

    // Execute the command if one was entered
    if (command.command != NULL) {
      if (strcmp(command.command, "exit") == 0) {
        free(input);
        freeCommand(&command);
        break;
      }
      executeCommand(command);
    }

    free(input);
    freeCommand(&command);
  }

  exit(0);
}

void displayPrompt() {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s$ ", cwd);
  } else {
    printf("$ ");
  }
  fflush(stdout);
}

char *getInput() {
  char *buffer = NULL;
  size_t size = 0;

  // Read a line from stdin
  if (getline(&buffer, &size, stdin) == -1) {
    free(buffer);
    return NULL;
  }

  // Remove trailing newline
  size_t len = strlen(buffer);
  if (len > 0 && buffer[len - 1] == '\n') {
    buffer[len - 1] = '\0';
  }

  return buffer;
}

// Helper function to append tokens
void addToken(char ***args, int *count, char *token) {
  *args = realloc(*args, sizeof(char *) * (*count + 1));
  (*args)[*count] = strdup(token);
  (*count)++;
}

struct ShellCommand parseInput(char *input) {
  struct ShellCommand cmd;
  cmd.command = NULL;
  cmd.arguments = NULL;
  cmd.numArgs = 0;
  cmd.inputFile = NULL;
  cmd.outputFile = NULL;

  if (input == NULL || strlen(input) == 0) {
    return cmd;
  }

  char **tokens = NULL;
  int numTokens = 0;

  char buffer[MAX_INPUT_LEN];
  int bufIdx = 0;
  int inQuotes = 0;
  int escapeNext = 0;

  for (int i = 0; input[i] != '\0'; i++) {
    char c = input[i];

    if (escapeNext) {
      buffer[bufIdx++] = c;
      escapeNext = 0;
    } else if (c == '\\') {
      escapeNext = 1;
    } else if (c == '"') {
      inQuotes = !inQuotes;
    } else if ((c == ' ' || c == '\t') && !inQuotes) {
      if (bufIdx > 0) {
        buffer[bufIdx] = '\0';
        addToken(&tokens, &numTokens, buffer);
        bufIdx = 0;
      }
    } else if ((c == '<' || c == '>') && !inQuotes && !escapeNext) {
      if (bufIdx > 0) {
        buffer[bufIdx] = '\0';
        addToken(&tokens, &numTokens, buffer);
        bufIdx = 0;
      }
      char sym[2] = {c, '\0'};
      addToken(&tokens, &numTokens, sym);
    } else {
      buffer[bufIdx++] = c;
    }
  }

  // Add final token if present
  if (bufIdx > 0) {
    buffer[bufIdx] = '\0';
    addToken(&tokens, &numTokens, buffer);
  }

  // Parse tokens to cmd struct structure
  for (int i = 0; i < numTokens; i++) {
    if (strcmp(tokens[i], "<") == 0) {
      if (i + 1 < numTokens) {
        cmd.inputFile = strdup(tokens[i + 1]);
        i++; // Skip next token
      }
    } else if (strcmp(tokens[i], ">") == 0) {
      if (i + 1 < numTokens) {
        cmd.outputFile = strdup(tokens[i + 1]);
        i++; // Skip next token
      }
    } else {
      // It's a command or argument
      if (cmd.command == NULL) {
        cmd.command = strdup(tokens[i]);
      }
      // Add as an argument too (execvp expects command as first arg)
      addToken(&(cmd.arguments), &(cmd.numArgs), tokens[i]);
    }
  }

  // Null terminate arguments needed for execvp
  if (cmd.arguments != NULL) {
    cmd.arguments = realloc(cmd.arguments, sizeof(char *) * (cmd.numArgs + 1));
    cmd.arguments[cmd.numArgs] = NULL;
  }

  // Free intermediate tokens array
  for (int i = 0; i < numTokens; i++) {
    free(tokens[i]);
  }
  free(tokens);

  return cmd;
}

void executeCommand(struct ShellCommand cmd) {
  // 1. Handle Built-in cd command
  if (strcmp(cmd.command, "cd") == 0) {
    if (cmd.numArgs < 2) {
      fprintf(stderr, "cd: missing argument\n");
    } else {
      if (chdir(cmd.arguments[1]) != 0) {
        fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
      }
    }
    return;
  }

  // 2. Default execution logic with fork and execvp
  pid_t pid = fork();

  if (pid < 0) {
    fprintf(stderr, "Fork failed\n");
    return;
  }

  if (pid == 0) {
    // Child process

    // 3. I/O Redirection
    if (cmd.inputFile != NULL) {
      int fd_in = open(cmd.inputFile, O_RDONLY);
      if (fd_in < 0) {
        fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
        exit(1);
      }
      dup2(fd_in, STDIN_FILENO);
      close(fd_in);
    }

    if (cmd.outputFile != NULL) {
      int fd_out = open(cmd.outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd_out < 0) {
        fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
        exit(1);
      }
      dup2(fd_out, STDOUT_FILENO);
      close(fd_out);
    }

    // Execute the command
    if (execvp(cmd.command, cmd.arguments) == -1) {
      fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
      exit(1);
    }
  } else {
    // Parent process
    // Wait for child to finish
    int status;
    waitpid(pid, &status, 0);
  }
}

void freeCommand(struct ShellCommand *cmd) {
  if (cmd->command != NULL) {
    free(cmd->command);
    cmd->command = NULL;
  }
  if (cmd->arguments != NULL) {
    for (int i = 0; i < cmd->numArgs; i++) {
      free(cmd->arguments[i]);
    }
    free(cmd->arguments);
    cmd->arguments = NULL;
  }
  if (cmd->inputFile != NULL) {
    free(cmd->inputFile);
    cmd->inputFile = NULL;
  }
  if (cmd->outputFile != NULL) {
    free(cmd->outputFile);
    cmd->outputFile = NULL;
  }
  cmd->numArgs = 0;
}
