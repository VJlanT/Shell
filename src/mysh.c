#include "shell.h"

int	main(void)
{
	char	*line = NULL;
	size_t	cap = 0;
	ssize_t	n;
	char	**words = NULL;

	while (1)
	{
		printf("%s", "> ");
		fflush(stdout);
		n = getline(&line, &cap, stdin);
		if (n == -1)
			break ;
		words = split_line(line);
		if (!words)
			break ;
		run_command(words);
		free(words);
	}
	free(line);
	return (0);
}
