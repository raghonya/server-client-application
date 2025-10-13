#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static int	check(char **s, int j)
{
	if (!s[j])
		while (--j >= 0)
			free(s[j]);
	return (j + 1);
}

static void	letter_count(const char *s, int *i, int *letcnt, char c)
{
	*letcnt = 0;
	while (s[*i] == c)
		(*i)++;
	while (s[*i] != c && s[*i] != 0)
	{
		(*letcnt)++;
		(*i)++;
	}
}

int	stralloc(char const *s, char **split, int wrdcnt, char c)
{
	int	i;
	int	j;
	int	letcnt;

	i = (!(j = -1));
	while (++j < wrdcnt)
	{
		while (s[i])
		{
			letter_count (s, &i, &letcnt, c);
			if (letcnt)
			{
				split[j] = malloc (sizeof(char) * (letcnt + 1));
				if (!check(split, j))
					return (0);
				break ;
			}
		}
	}
	return (1);
}

static char	**fill(char const *s, char **split, int wrdcnt, char c)
{
	int	i;
	int	j;
	int	let;

	i = 0;
	j = 0;
	while (j < wrdcnt)
	{
		let = 0;
		while (s[i])
		{
			while (s[i] == c)
				i++;
			while (s[i] != c && s[i] != 0)
				split[j][let++] = s[i++];
			split[j][let] = 0;
			break ;
		}
		j++;
	}
	split[j] = NULL;
	return (split);
}

char	**split(char const *s, char c)
{
	int		i;
	int		wrdcnt;
	char	**split;

	i = 0;
	wrdcnt = 0;
	while (s[i])
	{
		while (s[i] == c)
			i++;
		if (s[i] != 0)
			wrdcnt++;
		while (s[i] != c && s[i] != 0)
			i++;
	}
	split = malloc (sizeof(char *) * (wrdcnt + 1));
	if (!split)
		return (NULL);
	if (!stralloc(s, split, wrdcnt, c))
	{
		free (split);
		return (NULL);
	}
	return (fill(s, split, wrdcnt, c));
}
