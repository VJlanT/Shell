#include "shell.h"

void	run_command(char **words)
{
	pid_t	pid;

	if (!words || !*words)
		return ;
	pid = fork();
	if (pid < 0)
	{
		perror("fork");
		return ;
	}
	if (pid == 0)
	{
		if (execvp(words[0], words) < 0)
		{
			perror(words[0]);
			exit(1);
		}
	}
	else
		waitpid(pid, NULL, 0);
}
