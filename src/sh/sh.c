#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

int run_command(char* command)
{
	char *args[256];
	int i = 0;
	char *token = strtok(command, " ");
	while (token != NULL)
	{
		args[i++] = token;
		token = strtok(NULL, " ");
	}
	args[i] = NULL; // null-terminate the array
	pid_t pid = fork();
	if (pid == 0)
	{
		// Child process
		execvp(args[0], args);
		perror("execvp failed");
		exit(1);
	}
	else if (pid < 0)
	{
		perror("fork failed");
		return -1;
	}
	else
	{
		int status;
		// Parent process
		wait(&status); // wait for child to finish
		if (WIFEXITED(status))
		{
			return WEXITSTATUS(status); // return child's exit status
		}
		else if (WIFSIGNALED(status))
		{
			fprintf(stderr, "Child process terminated by signal %d\n", WTERMSIG(status));
			return -1;
		}
		else
		{
			fprintf(stderr, "Child process terminated abnormally\n");
			return -1;
		}
		return -1;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	/*
	printf("argc=%d\n", argc);
	for (int i = 0;i<argc;i++)
	{
		printf("argv[%d]=%s\n", i, argv[i]);
	}
	*/
	if (argc > 3)
	{
		if (strcmp(argv[1], "-c") == 0 && strcmp(argv[2], "--")==0)
		{
			return run_command(argv[3]);
		}
	}
	else
	while (1)
	{
		char cwd[256];
		getcwd(cwd, 256);
		printf("%s> ", cwd);
		fflush(stdout);
		char command[256];
		if (fgets(command, sizeof(command), stdin) == NULL)
		{
			break; // EOF
		}
		if (command[0] == '\n')
		{
			continue; // empty command
		}
		if (command[strlen(command) - 1] == '\n')
		{
			command[strlen(command) - 1] = '\0'; // remove newline
		}
		if (strcmp(command, "exit") == 0)
		{
			break; // exit command
		}
		if (strcmp(command, "help") == 0)
		{
			printf("Available commands: exit, help\n");
			continue;
		}
		run_command(command);
	}
	return 0;
}

