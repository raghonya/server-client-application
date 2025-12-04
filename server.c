#include "sc.h"

int				server_socket;
int volatile	server_running = 1;
int	volatile	active_clients;
pthread_mutex_t	cli_cnt_mutex;
fds				cli_fds[MAX_CLI_COUNT];

void	sig_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
	{
		server_running = 0;
		printf ("SIGINT received, closing the server!\n");
		close(server_socket);
		server_socket = -1;
		for (int i = 0; i < MAX_CLI_COUNT; ++i)
		{
			if (cli_fds[i].state == INACTIVE)
				continue ;
			printf ("Closing %d client\n", cli_fds[i].socket);
			send(cli_fds[i].socket, "Server is closed\r\n", 18, MSG_NOSIGNAL);
			close(cli_fds[i].socket);
		}
	}
}

void set_signal_action(struct sigaction *act)
{
	bzero(act, sizeof(*act));
	act->sa_handler = &sig_handler;
	sigaction(SIGINT, act, NULL);
	sigaction(SIGTERM, act, NULL);
}

void	change_clients_count(pthread_mutex_t *mut, int volatile *active_clients, int side)
{
	pthread_mutex_lock(mut);
	if (side == DECREASE)
		(*active_clients)--;
	else
		(*active_clients)++;
	pthread_mutex_unlock(mut);
}

void	rm_client_from_list(int socket)
{
	for (int i = 0; i < MAX_CLI_COUNT; ++i)
	{
		if (cli_fds[i].socket == socket)
		{
			close(socket);
			cli_fds[i].state = INACTIVE;
			return ;
		}
	}
}
void	add_client_to_list(int socket)
{
	for (int i = 0; i < MAX_CLI_COUNT; ++i)
	{
		if (cli_fds[i].state == INACTIVE)
		{
			cli_fds[i].socket = socket;
			cli_fds[i].state = ACTIVE;
			return ;
		}
	}
}

char	*create_response(int pipefd)
{
	char	*response;

	response = malloc(sizeof(char) * (BUF_SIZE + 1));
	if (!response)
	{
		close(pipefd);
		return (strdup(FAILED("memory")));
	}
	bzero(response, BUF_SIZE + 1);
	int read_cnt = read(pipefd, response, BUF_SIZE);
	if (read_cnt <= 0)
	{
		close(pipefd);
		free(response);
		return (strdup(FAILED("read")));
	}
	while (strstr(response, "\r\n") == NULL)
	{
		size_t	last_len = strlen(response);
		char *tmp = response;
		
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

char	*run_shell_command(char *full_command)
{
	int		cpid;
	int		pipefd[2];
	char	**to_exec;
	char	*response;

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
		printf ("Malloc error\n");
		return (strdup(FAILED("memory")));
	}
	printf ("fullcmd: '%s'\n", full_command);
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
	{
		// free(full_command);
		// free_2d_array(to_exec);
		close(pipefd[0]);
		if (dup2(pipefd[1], STDOUT_FILENO) < 0 || dup2(pipefd[1], STDERR_FILENO) < 0)
		{
			printf ("Dup2 error");
			exit (1);
		}
		close(pipefd[1]);
		exit(execvp(to_exec[0], to_exec));
	}
	else
	{
		int		result;
		int		status;
		struct timeval	time_start;
		struct timeval	time_current;
		
		free_2d_array(to_exec);
		gettimeofday(&time_start, NULL);
		while (1)
		{
			result = waitpid(cpid, &status, WNOHANG);
			if (result == cpid)
				break;
			else if (result == 0)
			{
				gettimeofday(&time_current, NULL);
				long timeout = (time_current.tv_sec - time_start.tv_sec) * 1000 + (time_current.tv_usec - time_start.tv_usec) / 1000;
				if (timeout / 1000 >= TIMEOUT_SEC)
				{
					kill(cpid, SIGKILL);
					waitpid(cpid, &status, 0);
					close(pipefd[1]);
					close(pipefd[0]);
					return (strdup(FAILED("timeout")));
				}
				usleep(50000);
			}
			else
			{
				kill(cpid, SIGKILL);
				waitpid(cpid, &status, 0);
				close(pipefd[1]);
				close(pipefd[0]);
				return (strdup(FAILED("execute")));
			}
		}
		if (WIFEXITED(status))
		{
			status = WEXITSTATUS(status);
			if (status)
			{
				close(pipefd[1]);
				close(pipefd[0]);
				return (strdup(FAILED("execute")));
			}
		}
		write(pipefd[1], "\r\n", 2);
		close(pipefd[1]);
		response = create_response(pipefd[0]);
	}
	return (response);
}

int	parse_command(data_t *client)
{
	int		ret_code;
	char	**splitted;

	ret_code = SUCCESS;
	splitted = split(client->request, ' ');
	if (!splitted)
		return (ERROR);
	if (splitted[0] && strcmp(splitted[0], "disconnect") == 0)
		ret_code = DISCONNECT;
	else if (splitted[0] && strcmp(splitted[0], "shell") == 0)
	{
		if (!splitted[1] || splitted[1][0] != '"')
			ret_code = ERROR;
		else
		{
			int	i;
			int	from_start;

			from_start = 0;
			for (i = 0; client->request[i] && client->request[i] != '"'; ++i)
				from_start++;
			for (i = from_start + 1; client->request[i] && client->request[i] != '"'; ++i)
				;
			if (!client->request[i])
				ret_code = ERROR;
			else
			{
				client->request[i] = 0;
				// printf ("sending to execute '%s'\n", client->request + from_start + 1);
				client->response = run_shell_command(client->request + from_start + 1);
			}
		}
	}
	else
		ret_code = ERROR;
	free_2d_array(splitted);
	return (ret_code);
}

void	*client_handler(void *p_socket)
{
	int		socket = *((int *)(p_socket));
	int		read_cnt;
	data_t	data;

	while (server_running)
	{
		data.response = NULL;
		data.request = malloc(sizeof(char) * (BUF_SIZE + 1));
		if (!data.request)
		{
			printf ("Malloc error\n");
			rm_client_from_list(socket);
			change_clients_count(&cli_cnt_mutex, &active_clients, DECREASE);
			break ;
		}
		bzero(data.request, BUF_SIZE + 1);
		read_cnt = recv(socket, data.request, BUF_SIZE, MSG_NOSIGNAL);
		if  (read_cnt <= 0)
		{
			printf ("Client %d disconnected\n", socket);
			// close(socket);
			free(data.request);
			rm_client_from_list(socket);
			change_clients_count(&cli_cnt_mutex, &active_clients, DECREASE);
			break ;
		}
		while (strstr(data.request, "\r\n") == NULL)
		{
			size_t	last_len = strlen(data.request);
			char *tmp = data.request;
			data.request = realloc(data.request, last_len + read_cnt + 1);
			if (!data.request)
			{
				printf ("Malloc error\n");
				free(tmp);
				rm_client_from_list(socket);
				change_clients_count(&cli_cnt_mutex, &active_clients, DECREASE);
				return (NULL);
			}
			read_cnt = recv(socket, data.request + last_len, BUF_SIZE, MSG_NOSIGNAL);
			if (read_cnt < 0)
			{
				printf ("Connection closed\n");
				free(data.request);
				rm_client_from_list(socket);			
				change_clients_count(&cli_cnt_mutex, &active_clients, DECREASE);
				return (NULL);
			}
			data.request[last_len + read_cnt] = 0;
		}
		int	ret_code = parse_command(&data);
		if (data.request) free(data.request);
		if (ret_code == DISCONNECT)
		{
			printf ("CLient %d disconnected via command\n", socket);
			send(socket, "Disconnected from the server\r\n", 30, MSG_NOSIGNAL);
			rm_client_from_list(socket);			
			change_clients_count(&cli_cnt_mutex, &active_clients, DECREASE);
			break ;
		}
		else if (ret_code == ERROR)
			data.response = strdup("Wrong command format\r\n");
		send(socket, data.response, strlen(data.response), MSG_NOSIGNAL);
		if (data.response) free(data.response);
	}
	return (NULL);
}

int main()
{
	struct sigaction	act;
	struct sockaddr_in	servaddr;
	struct sockaddr_in	cli;
	socklen_t			cli_len;
	int					client_socket;
	pthread_t			*client_thread;

	// sigaction()
	set_signal_action(&act);
	cli_len = sizeof(cli);
	// bzero(cli_fds, sizeof(cli_fds));
	pthread_mutex_init(&cli_cnt_mutex, NULL);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		printf("socket creation failed\n");
		exit(1);
	}
	else printf("Socket successfully created\n");

	int	opt = 0;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	// fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL, 0) | O_NONBLOCK);
	if ((bind(server_socket, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{
		printf("socket bind failed\n");
		exit(1);
	}
	else printf("Socket successfully binded\n"); 

	if ((listen(server_socket, MAX_CLI_COUNT)) != 0)
	{
		printf("Listen failed\n");
		exit(1);
	}

	while (server_running)
	{
		client_socket = accept(server_socket, (struct sockaddr *)&cli, &cli_len);
		if (client_socket < 0)
		{
			printf ("Accept error\n");
			continue ;
		}
		pthread_mutex_lock(&cli_cnt_mutex);
		if (active_clients >= MAX_CLI_COUNT)
		{
			send(client_socket, "Busy\r\n", 6, MSG_NOSIGNAL);
			close(client_socket);
			pthread_mutex_unlock(&cli_cnt_mutex);
			continue ;
		}
		add_client_to_list(client_socket);
		active_clients++;
		pthread_mutex_unlock(&cli_cnt_mutex);
		printf ("New client connected\n");
		int		*new_client_socket = malloc(sizeof(int));
		if (!new_client_socket)
		{
			printf ("Malloc error\n");
			// close(client_socket);
			rm_client_from_list(client_socket);
			change_clients_count(&cli_cnt_mutex, &active_clients, DECREASE);
			continue ;
		}
		client_thread = malloc(sizeof(pthread_t));
		if (!client_thread)
		{
			free(new_client_socket);
			rm_client_from_list(client_socket);
			change_clients_count(&cli_cnt_mutex, &active_clients, DECREASE);
			continue ;
		}
		*new_client_socket = client_socket;
		if (pthread_create(client_thread, NULL, &client_handler, new_client_socket) != 0)
		{
			printf ("Pthread failed\n");
			free(new_client_socket);
			free(client_thread);
			rm_client_from_list(client_socket);
			change_clients_count(&cli_cnt_mutex, &active_clients, DECREASE);
			continue ;
		}
		pthread_detach(*client_thread);
		free(client_thread);
	}
	if (server_socket != -1)
		close(server_socket);
	printf ("Server closed!!!\n");
	return (0);

}
