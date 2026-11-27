#include "shell.h"

static int	is_sep(char c)
{
	return (c == '\0' || c == ' ' || (c >= 9 && c <= 13));
}

static int	is_start(char *s, int i)
{
	return (!is_sep(s[i]) && (i == 0 || is_sep(s[i - 1])));
}

static int	count_words(char *s)
{
	int	i = 0;
	int	count = 0;

	while (s[i])
	{
		if (is_start(s, i))
			count++;
		i++;
	}
	return (count);
}

static void	fill_tab(char **words, char *s)
{
	int	i = 0;
	int	k = 0;

	while (s[i])
	{
		if (is_start(s, i))
			words[k++] = &s[i];
		else if (is_sep(s[i]))
			s[i] = '\0';
		i++;
	}
	words[k] = NULL;
}

char	**split_line(char *line)
{
	char	**words = NULL;
	int	size = 0;

	size = count_words(line);
	words = malloc(sizeof(char *) * (size + 1));
	if (!words)
		return (NULL);
	fill_tab(words, line);
	return (words);
}
