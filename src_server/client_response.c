#include "server.h"

void	execute_command(char **binary_args, int *pipefd)
{
	close(pipefd[0]);
	if (dup2(pipefd[1], STDOUT_FILENO) < 0 || dup2(pipefd[1], STDERR_FILENO) < 0)
	{
		printf ("Dup2 error");
		exit (1);
	}
	close(pipefd[1]);
	exit(execvp(binary_args[0], binary_args));
}

char	*create_response(int pipefd)
{
	char	*response;
	char	*tmp;
	size_t	last_len;
	int		read_cnt;

	response = calloc(BUF_SIZE + 1, sizeof(char));
	if (!response)
	{
		close(pipefd);
		return (strdup(FAILED("memory")));
	}
	read_cnt = read(pipefd, response, BUF_SIZE);
	if (read_cnt <= 0)
	{
		free(response);
		close(pipefd);
		return (strdup(FAILED("read")));
	}
	while (read_cnt == BUF_SIZE || strstr(response, "\r\n") == NULL)
	{
		last_len = strlen(response);
		tmp = response;

		response = realloc(response, last_len + read_cnt + 1);
		if (!response)
		{
			free(tmp);
			close(pipefd);
			return (strdup(FAILED("memory")));
		}
		read_cnt = read(pipefd, response + last_len, BUF_SIZE);
		if (read_cnt < 0)
		{
			free(response);
			close(pipefd);
			return (strdup(FAILED("read")));
		}
		response[last_len + read_cnt] = 0;
	}
	return (response);
}

int	check_timeout(pid_t cpid, int *status, int *pipefd)
{
	int				result;
	long			timeout;
	struct timeval	time_start;
	struct timeval	time_current;

	gettimeofday(&time_start, NULL);
	while (1)
	{
		result = waitpid(cpid, status, WNOHANG);
		if (result == cpid)
			break ;
		else if (result == 0)
		{
			gettimeofday(&time_current, NULL);
			timeout = (time_current.tv_sec - time_start.tv_sec) * 1000 + (time_current.tv_usec - time_start.tv_usec) / 1000;
			if (timeout / 1000 >= TIMEOUT_SEC)
			{
				kill(cpid, SIGKILL);
				waitpid(cpid, status, 0);
				close(pipefd[1]);
				close(pipefd[0]);
				return (1);
			}
			usleep(50000);
		}
		else
		{
			kill(cpid, SIGKILL);
			waitpid(cpid, status, 0);
			close(pipefd[1]);
			close(pipefd[0]);
			return (2);
		}
	}
	return (0);
}

int	check_exit_status(pid_t cpid, int *pipefd)
{
	int				result;
	int				status;

	result = check_timeout(cpid, &status, pipefd);
	if (result)
		return (result);
	if (WIFEXITED(status))
	{
		status = WEXITSTATUS(status);
		if (status)
		{
			close(pipefd[1]);
			close(pipefd[0]);
			return (2);
		}
	}
	return (0);
}

char	*run_shell_command(char *full_command)
{
	int		cpid;
	int		pipefd[2];
	char	**to_exec;
	char	*response;
	int		child_result;

	response = NULL;
	to_exec = NULL;
	if (pipe(pipefd) < 0)
	{
		printf ("Pipe error\n");
		return (strdup(FAILED("execute")));
	}
	to_exec = split(full_command, ' ');
	if (!to_exec)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (strdup(FAILED("memory")));
	}
	cpid = fork();
	if (cpid < 0)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		printf ("Fork error\n");
		free(to_exec);
		return (strdup(FAILED("execute")));
	}
	else if (cpid == 0)
		execute_command(to_exec, pipefd);
	else
	{
		free_2d_array(to_exec);
		child_result = check_exit_status(cpid, pipefd);
		if (child_result == 1)
			return (strdup(FAILED("timeout")));
		else if (child_result == 2)
			return (strdup(FAILED("execute")));
		write(pipefd[1], "\r\n", 2);
		close(pipefd[1]);
		response = create_response(pipefd[0]);
	}
	return (response);
}

int		validate_command(char *cmd, int *start_index)
{
	int		i;

	for (i = 0; cmd[i] && cmd[i] != '"'; ++i)
		(*start_index)++;
	for (i = *start_index + 1; cmd[i] && cmd[i] != '"'; ++i)
		;
	if (!cmd[i])
		return (ERROR);
	cmd[i] = 0;

	return (SUCCESS);
}

int	parse_command(data_t *client)
{
	int		start_index;
	char	**splitted;
	
	start_index = 0;
	splitted = split(client->request, ' ');
	if (!splitted || !splitted[0])
		return (ERROR);
	if (strcmp(splitted[0], "disconnect") == 0)
	{
		free_2d_array(splitted);
		return (DISCONNECT);
	}
	else if (strcmp(splitted[0], "shell") == 0)
	{
		if (!splitted[1] || splitted[1][0] != '"' \
		|| validate_command(client->request, &start_index) == ERROR)
		{
			free_2d_array(splitted);
			return (ERROR);
		}
		client->response = run_shell_command(client->request + start_index + 1);
	}
	else
	{
		free_2d_array(splitted);
		return (ERROR);
	}
	free_2d_array(splitted);
	return (SUCCESS);
}
