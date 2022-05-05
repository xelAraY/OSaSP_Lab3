#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<dirent.h>
#include<fcntl.h>

#define MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

int RedirectHandler(const char* fileName, int handle, int mode);
int ProcessRedirection(char* commandArguments[], int argCount);
int ReadAllCommands(struct dirent*** commands); 
int ReadNextCommand(char* argv[], int argc, int* FirsArg, int* currentArgNum, struct dirent** commands, int allCommands);
int Filter(const struct dirent* filePtr);
int ExecuteCommand(char** dir, char* commandArguments[]);
int WaitChild(pid_t childPid);

int main(int argc, char* argv[])
{
  if (argc < 2){
    fprintf(stderr, "Error. Incorrect input: ./*fileName* *Command*\n*fileName*: Name of executable file\n*Command*: Command to execute\n");
    return 1;
  }
  struct dirent** commands;
  int allCommands = ReadAllCommands(&commands);
  int currentArgNum = 1, firstArgNum = 1;
  while (currentArgNum < argc){
    if (ReadNextCommand(argv, argc, &firstArgNum, &currentArgNum, commands, allCommands) != 0){
      fprintf(stderr, "Could not read commnad\n");
      return 2;
    }

    char* commandArguments[currentArgNum - firstArgNum + 1];
    for (int i = 0; i < currentArgNum - firstArgNum; i++){
      commandArguments[i] = argv[i + firstArgNum]; 
    }
    commandArguments[currentArgNum - firstArgNum] = NULL;

    char* directory = calloc(1000, sizeof(char));
    if (directory == NULL){
      perror("Error. Couldnt create variable\n");
      return 3;
    }
    strcat(directory, "/bin/");
    strcat(directory, commandArguments[0]);
    ExecuteCommand(&directory, commandArguments); 
    
    free(directory);
  } 

  for (int i = 0; i < allCommands; i++){
    free(commands[i]);
  }
  free(commands);
  return 0;
}

int RedirectHandler(const char* fileName, int handle, int mode)
{
  int fileDescriptor = open(fileName, O_CREAT | mode, MODE);
  if (fileDescriptor == -1){
    fprintf(stderr, "Could not open file \"%s\"\n", fileName);
    return -1;
  }
  
  if (dup2(fileDescriptor, handle) < 0) {
    close(fileDescriptor);
    fprintf(stderr, "Could not override handle \"%d\"\n", handle);
    return -2;
  }

  close(fileDescriptor);
  return handle;
}

int ProcessRedirection(char* commandArguments[], int argCount)
{
  int offset = 0;
  int result = -3;
  if (strstr(commandArguments[argCount-1], ">>")){
      result = RedirectHandler(&(commandArguments[argCount-1][2]), STDOUT_FILENO, O_WRONLY | O_APPEND);  
  }

  if (strstr(commandArguments[argCount-1], ">")){
      result = RedirectHandler(&(commandArguments[argCount-1][1]), STDOUT_FILENO, O_WRONLY | O_TRUNC);  
  }

  if (strstr(commandArguments[argCount-1], "<")){
      result = RedirectHandler(&(commandArguments[argCount-1][1]), STDIN_FILENO, O_RDONLY);  
  }

  if (result > 0){
    commandArguments[argCount-1] = NULL;
  }
  return result;
}

int ReadAllCommands(struct dirent*** commands)
{
  int result = scandir("/bin", commands, Filter, NULL);
  if (result == -1){
    perror("Could not create commands list");
  }
  return result;
}

int Filter(const struct dirent* filePtr)
{
  if (filePtr->d_type == DT_REG){
    return 1;
  }
  return 0;
}

int ReadNextCommand(char* argv[], int argc, int* firstArgNum, int* currentArgNum, struct dirent** commands, int allCommands)
{
  int argCount = 2; 
  int currArgNum = 0;
  int result = 0;

  while (result == 0 && *currentArgNum < argc){
    for (int i = 0; i < allCommands && result == 0; i++){
      if (strcmp(argv[*currentArgNum], commands[i]->d_name) == 0){
        result = 1;
      }
    }
    if (result == 1){
      *firstArgNum = *currentArgNum;
    }
    (*currentArgNum)++;
  }

  if (result == 0 && *currentArgNum == argc){
    fprintf(stderr, "Could not find command\n");
    return 1;  
  }

  result = 0;
  while (result == 0 && *currentArgNum < argc){
    for (int i = 0; i < allCommands && result == 0; i++){
      if (strcmp(argv[*currentArgNum], commands[i]->d_name) == 0){
        result = 1;
      }
    } 
    (*currentArgNum)++;
  }
  if (result == 1){
    (*currentArgNum)--;
  }
  return 0;
}

int ExecuteCommand(char** dir, char* commandArguments[])
{
  int argCount = 0;
  while (commandArguments[argCount] != NULL) argCount++;
  int stdInDescriptor = dup(STDIN_FILENO);
  int stdOutDescriptor = dup(STDOUT_FILENO);
  int result = ProcessRedirection(commandArguments, argCount);
  if (result <= 0 && result != -3){
    close(stdInDescriptor);
    close(stdOutDescriptor);
    return 1;
  }

  pid_t childPid = fork();
  switch (childPid){
    case -1:
	  perror("Child process could not be created\n");
      return 1;
      break;
    case 0:
      if (execvp(*dir, commandArguments) == -1){
        perror("Execute command failue");
        return 2;
      }
      break;
    default:
      if (WaitChild(childPid) == 1){
        return 1;
      }
      if (result != -3){
        switch(result){
          case 0:
            result = dup2(stdInDescriptor, STDIN_FILENO);
            break;
          case 1:
            result = dup2(stdOutDescriptor, STDOUT_FILENO);
            break;
        } 
        if (result < 0){
          close(stdInDescriptor);
          close(stdOutDescriptor);
          fprintf(stderr, "Could not return overrited handle\n");
          return 3;
        }
      }
      close(stdInDescriptor);
      close(stdOutDescriptor);
      break;
  }
}

int WaitChild(pid_t childPid) 
{
	if (waitpid(childPid, NULL, 0) == -1)
		perror("Wait pid failue\n");
}