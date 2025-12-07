#include "sc.h"

int				g_server_socket;
int volatile	g_server_running = 1;
int	volatile	g_active_clients;
pthread_mutex_t	g_cli_cnt_mutex;
cli_t			g_clients[MAX_CLI_COUNT];

void	sig_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
	{
		g_server_running = 0;
		printf ("SIGINT received, closing the server!\n");
		close(g_server_socket);
		g_server_socket = -1;
		for (int i = 0; i < MAX_CLI_COUNT; ++i)
		{
			if (g_clients[i].state == INACTIVE)
				continue ;
			printf ("Closing %d client\n", g_clients[i].socket);
			send(g_clients[i].socket, "Server is closed\r\n", 18, MSG_NOSIGNAL);
			close(g_clients[i].socket);
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

void	change_clients_count(pthread_mutex_t *mut, int side)
{
	pthread_mutex_lock(mut);
	if (side == DECREASE)
		g_active_clients--;
	else
		g_active_clients++;
	pthread_mutex_unlock(mut);
}

int		get_client_index(int socket)
{
	for (int i = 0; i < MAX_CLI_COUNT; ++i)
		if (g_clients[i].socket == socket)
			return (i);
	return (-1);
}

void	rm_client_from_list(int socket)
{
	for (int i = 0; i < MAX_CLI_COUNT; ++i)
	{
		if (g_clients[i].socket == socket)
		{
			close(socket);
			g_clients[i].state = INACTIVE;
			return ;
		}
	}
	change_clients_count(&g_cli_cnt_mutex, DECREASE);
}

void	add_client_to_list(int socket)
{
	for (int i = 0; i < MAX_CLI_COUNT; ++i)
	{
		if (g_clients[i].state == INACTIVE)
		{
			g_clients[i].socket = socket;
			g_clients[i].state = ACTIVE;
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
		free(response);
		close(pipefd);
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
	{
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
		int				result;
		int				status;
		long			timeout;
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
				timeout = (time_current.tv_sec - time_start.tv_sec) * 1000 + (time_current.tv_usec - time_start.tv_usec) / 1000;
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

int	parse_command(t_data *client)
{
	int		start_index;
	char	**splitted;
	
	start_index = 0;
	splitted = split(client->request, ' ');
	if (!splitted)
		return (ERROR);
	if (splitted[0] && strcmp(splitted[0], "disconnect") == 0)
	{
		free_2d_array(splitted);	
		return (DISCONNECT);
	}
	else if (splitted[0] && strcmp(splitted[0], "shell") == 0)
	{
		if (!splitted[1] || splitted[1][0] != '"' || validate_command(client->request, &start_index) == ERROR)
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

void	*client_handler(void *cli_info)
{
	cli_t	*client = (cli_t *)(cli_info);
	int		read_cnt;
	t_data	data = client->data;

	while (g_server_running)
	{
		data.response = NULL;
		data.request = malloc(sizeof(char) * (BUF_SIZE + 1));
		if (!data.request)
		{
			printf ("Malloc error\n");
			rm_client_from_list(client->socket);
			break ;
		}
		bzero(data.request, BUF_SIZE + 1);
		read_cnt = recv(client->socket, data.request, BUF_SIZE, MSG_NOSIGNAL);
		if  (read_cnt <= 0)
		{
			printf ("Client %d disconnected\n", client->socket);
			// close(client->socket);
			free(data.request);
			rm_client_from_list(client->socket);
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
				rm_client_from_list(client->socket);
				return (NULL);
			}
			read_cnt = recv(client->socket, data.request + last_len, BUF_SIZE, MSG_NOSIGNAL);
			if (read_cnt < 0)
			{
				printf ("Connection closed\n");
				free(data.request);
				rm_client_from_list(client->socket);			
				return (NULL);
			}
			data.request[last_len + read_cnt] = 0;
		}
		int	ret_code = parse_command(&data);
		if (data.request) free(data.request);
		if (ret_code == DISCONNECT)
		{
			printf ("CLient %d disconnected via command\n", client->socket);
			send(client->socket, "Disconnected from the server\r\n", 30, MSG_NOSIGNAL);
			rm_client_from_list(client->socket);			
			break ;
		}
		else if (ret_code == ERROR)
			data.response = strdup("Wrong command format\r\n");
		send(client->socket, data.response, strlen(data.response), MSG_NOSIGNAL);
		if (data.response) free(data.response);
	}
	return (NULL);
}

int	create_server(struct sockaddr_in *servaddr)
{
	int	opt;
	int	server_socket;

	opt = 0;
	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr->sin_port = htons(PORT);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		printf("socket creation failed\n");
		exit(1);
	}
	else printf("Socket successfully created\n");

	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if ((bind(server_socket, (struct sockaddr *)servaddr, sizeof(*servaddr))) != 0)
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

	return (server_socket);
}

int	accept_client()
{
	int					client_socket;
	struct sockaddr_in	cli;
	socklen_t			cli_len;

	cli_len = sizeof(cli);
	client_socket = accept(g_server_socket, (struct sockaddr *)&cli, &cli_len);
	if (client_socket < 0)
	{
		printf ("Accept error\n");
		return (-1);
	}
	pthread_mutex_lock(&g_cli_cnt_mutex);
	if (g_active_clients >= MAX_CLI_COUNT)
	{
		send(client_socket, "Busy\r\n", 6, MSG_NOSIGNAL);
		close(client_socket);
		pthread_mutex_unlock(&g_cli_cnt_mutex);
		return (-1) ;
	}
	add_client_to_list(client_socket);
	g_active_clients++;
	pthread_mutex_unlock(&g_cli_cnt_mutex);

	return (client_socket);
}

int	create_client_handler(int client_socket)
{
	int		*tmp_cli_sock;
	int		index;

	tmp_cli_sock = malloc(sizeof(int));
	if (!tmp_cli_sock)
	{
		puts ("Malloc failed");
		rm_client_from_list(client_socket);
		return (1);
	}
	*tmp_cli_sock = client_socket;
	index = get_client_index(client_socket);
	if (index < 0)
	{
		puts("Client socket not found");
		return (1);
	}
	if (pthread_create(&g_clients[index].handler, NULL, &client_handler, &g_clients[index]) != 0)
	{
		puts ("Pthread failed");
		free(tmp_cli_sock);
		rm_client_from_list(client_socket);
		return (1);
	}
	pthread_detach(g_clients[index].handler);

	return (0);
}

void	init_variables(struct sigaction *act, struct sockaddr_in *servaddr)
{
	set_signal_action(act);
	bzero(g_clients, sizeof(g_clients));
	pthread_mutex_init(&g_cli_cnt_mutex, NULL);
	bzero(servaddr, sizeof(*servaddr));
	g_server_socket = create_server(servaddr);
}

int main()
{
	struct sigaction	act;
	struct sockaddr_in	servaddr;
	int					client_socket;

	init_variables(&act, &servaddr);
	while (g_server_running)
	{
		client_socket = accept_client();
		if (client_socket < 0)
			continue ;
		printf ("New client connected\n");
		if (create_client_handler(client_socket) != 0)
			continue ;
	}
	if (g_server_socket != -1)
		close(g_server_socket);
	printf ("Server closed!!!\n");

	return (0);
}
