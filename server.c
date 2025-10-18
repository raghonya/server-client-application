#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT			7744
#define BUF_SIZE		100
#define INCREASE		1
#define DECREASE		0
#define TIMEOUT_SEC		3

typedef enum error_codes_t
{
	SUCCESS = 0,
	DISCONNECT,
	ERROR
} error_codes_t;

char	**split(char *, char);
void	free_2d_array(char **);

typedef struct data_t
{
	char	*request;
	char	*response;
} data_t;

int				server_socket;
int volatile	server_running = 1;

void	sig_handler(int signum)
{
	if (signum == SIGINT)
	{
		server_running = 0;
		printf ("SIGINT received, closing the server!\n");
		close(server_socket);
	}
}

void	change_clients_count(pthread_mutex_t *mut, int *cli_cnt, int side)
{
	pthread_mutex_lock(mut);
	if (side == DECREASE)
		(*cli_cnt)--;
	else
		(*cli_cnt)++;
	pthread_mutex_unlock(mut);
}

char	**run_shell_command(char *full_command)
{
	int		cpid;
	int		pipefd[2];
	char	**to_exec;
	char	*response;

	response = NULL;
	to_exec = split(full_command, ' ');
	if (!to_exec)
	{
		printf ("Malloc error\n");
		return (strdup("Malloc error"));
	}
	if (pipe(pipefd) < 0)
	{
		printf ("Pipe error\n");
		return (strdup("Pipe error"));
	}
	cpid = fork();
	if (cpid < 0)
	{
		printf ("Fork error\n");
		return (strdup("Fork error"));
	}
	if (cpid == 0)
	{
		close(pipefd[0]);
		if (dup2(pipefd[1], STDOUT_FILENO) < 0 || dup2(pipefd[1], STDERR_FILENO))
		{
			printf ("Dup2 error\n");
			exit (1);
		}
		close(pipefd[1]);
		execvp(to_exec[0], to_exec);
		exit(1);
	}
	else
	{
		close(pipefd[1]);
		while (1)
		{
			int		result;
			struct timespec	time_start;
			struct timespec	time_current;

			gettimeofday(&time_start, NULL);
			result = waitpid(cpid, NULL, WNOHANG);
			if (result == cpid)
				break;
			else if (result == 0)
			{
				gettimeofday(&time_current, NULL);
				long timeout = (time_current.tv_sec - time_start.tv_sec) * 1000 + (time_current.tv_usec - time_start.tv_usec) / 1000;
				if (timeout / 1000 >= TIMEOUT_SEC)
				{
					kill(cpid, SIGKILL);
					waitpid(cpid, NULL, 0);
					close(pipefd[0]);	
					return (strdup("Timeout"));
				}
				usleep(50000);
			}
			else
			{
				kill(cpid, SIGKILL);
				waitpid(cpid, NULL, 0);
				close(pipefd[0]);	
				return (strdup("Failed to get execution data"));
			}
		}
		response = malloc(sizeof(char) * (BUF_SIZE + 1));
		if (!response)
		{
			close(pipefd[0]);
			return (strdup("Malloc error"));
		}
		int read_cnt = read(pipefd[0], response, BUF_SIZE);
		if (read_cnt <= 0)
		{
			close(pipefd[0]);
			return (strdup("Read error"));
		}
		if (read_cnt == BUF_SIZE)
		{
			while (read_cnt > 0)
			{
				response = realloc(response, shifting(response, read_cnt) + 1);
				if (!response)
				{
					free(response);
					close(pipefd[0]);
					return (strdup("Malloc error"));
				}
				read_cnt = read(socket, response + strlen(response), BUF_SIZE);
				response[shifting(response, read_cnt)] = 0;
				// printf ("after recv %d errno %d\n", read_cnt, errno);
			}
		}
	}
	return (response);
}

int	parse_command(data_t *client)
{
	int		ret_code;
	char	**splitted;

	ret_code = 0;
	splitted = split(client->request, ' ');
	if (!splitted)
	return (2);
	printf ("%s\n", splitted[0]);
	if (splitted[0] && strcmp(splitted[0], "disconnect") == 0)
		ret_code = 1;
	else if (splitted[0] && strcmp(splitted[0], "shell") == 0)
	{
		if (!splitted[1] || splitted[1][0] != '"')
			ret_code = 2;
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
				ret_code = 2;
			else
			{
				client->request[i] = 0;
				printf ("sedning to execute '%s'\n", client->request + from_start + 1);
				// response = run_shell_command(client->request + from_start + 1, &client->response);
				client->response = run_shell_command(client->request + from_start + 1);
			}
		}
	}
	free_2d_array(splitted);
	return (ret_code);
}

size_t	shifting(char *buf, int count)
{
	return (strlen(buf) + count);
}

void	*client_handler(void *p_socket)
{
	int		socket = *((int *)(p_socket));
	int		read_cnt;
	data_t	data;

	free(p_socket);
	while (server_running)
	{
		data.request = malloc(sizeof(char) * (BUF_SIZE + 1));
		if (!data.request)
		{
			printf ("Malloc error\n");
			close(socket);
			exit (1);
		}
		bzero(data.request, BUF_SIZE + 1);
		read_cnt = recv(socket, data.request, BUF_SIZE, 0);
		if  (read_cnt <= 0)
		{
			printf ("Client disconnected\n");
			close(socket);
			free(data.request);
			// return (NULL);
			break ;
		}
		printf ("readcnt: %d, buf: '%s'\n", read_cnt, data.request);
		if (read_cnt == BUF_SIZE)
		{
			while (read_cnt > 0)
			{
				printf ("bbbbbbb\n");
				data.request = realloc(data.request, shifting(data.request, read_cnt) + 1);
				if (!data.request)
				{
					printf ("Malloc error\n");
					free(data.request);
					close(socket);
					exit (1);
				}
				printf ("until recv\n");
				read_cnt = recv(socket, data.request + strlen(data.request), BUF_SIZE, 0);
				data.request[shifting(data.request, read_cnt)] = 0;
				// printf ("after recv %d errno %d\n", read_cnt, errno);
			}
		}
		parse_command(&data);
		// printf ("request: '%s'\n", data.request);
		// send(socket, data.request, strlen(data.request), MSG_NOSIGNAL);
	}
	// data.request[]
	return (NULL);
}

void set_signal_action(void)
{
	struct sigaction act;

	bzero(&act, sizeof(act));
	act.sa_handler = &sig_handler;
	sigaction(SIGINT, &act, NULL);
}


int main()
{
	struct sockaddr_in	servaddr;

	struct sockaddr_in	cli;
	socklen_t			cli_len;
	int					client_socket;
	pthread_mutex_t		cli_cnt_mutex;
	pthread_t			*client_thread;
	int					cli_cnt;

	// sigaction()
	set_signal_action();
	cli_cnt = 0;
	cli_len = sizeof(cli);
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

	int	opt;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if ((bind(server_socket, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{
		printf("socket bind failed\n");
		exit(1);
	}
	else printf("Socket successfully binded\n"); 

	if ((listen(server_socket, 5)) != 0)
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
		if (cli_cnt > 5)
		{
			printf ("Server supports only 5 clients at once, try connecting later\n");
			send(client_socket, "Busy\0", 5, MSG_NOSIGNAL);
			close(client_socket);
			pthread_mutex_unlock(&cli_cnt_mutex);
			continue ;
		}
		cli_cnt++;
		pthread_mutex_unlock(&cli_cnt_mutex);
		printf ("New client connected\n");
		int		*new_client_socket = malloc(sizeof(int));
		if (!new_client_socket)
		{
			printf ("Malloc error\n");
			close(client_socket);
			change_clients_count(&cli_cnt_mutex, &cli_cnt, DECREASE);
			continue ;
		}
		client_thread = malloc(sizeof(pthread_t));
		if (!client_thread)
		{
			close(client_socket);
			free(new_client_socket);
			change_clients_count(&cli_cnt_mutex, &cli_cnt, DECREASE);
			continue ;
		}
		*new_client_socket = client_socket;
		if (pthread_create(client_thread, NULL, &client_handler, new_client_socket) != 0)
		{
			printf ("Pthread failed\n");
			close(*new_client_socket);
			free(new_client_socket);
			free(client_thread);
			change_clients_count(&cli_cnt_mutex, &cli_cnt, DECREASE);
			continue ;
		}
		pthread_detach(*client_thread);
		free(client_thread);
	}
	printf ("Server closed!!!\n");
	return (0);
		// int	ret = select(max_fd + 1, &read_fds, &write_fds, &exception_fds, NULL /*timeout*/);
		// if (ret == 0)
		// {
		// 	printf ("Timeout\n");
		// 	continue ;
		// }
		// else if (ret < 0)
		// {
		// 	printf ("Socket error\n");
		// 	continue ;
		// }
		// if (FD_ISSET(server_socket, &read_fds))
		// {
			
		// 	cli_len = sizeof(cli);	
		// 	client_socket = accept(server_socket, (struct sockaddr *)&cli, &cli_len);
		// 	if (client_socket < 0)
		// 		printf ("Accept failed\n");
		// 	else
		// 	{
		// 		printf ("New client connected with socket %d\n", client_socket);
		// 		FD_SET(client_socket, &master_read_fds);
		// 		max_fd = (max_fd >= client_socket) ? max_fd : client_socket;
		// 	}
		// 	continue ;
		// }
		// for (int fd = 3; fd < max_fd + 1; ++fd)
		// {
		// 	if (FD_ISSET(fd, &read_fds))
		// 	{
		// 		bzero(clients[fd].request, BUF_SIZE);
		// 		int read_cnt = recv(fd, clients[fd].request, BUF_SIZE, 0);
		// 		if (read_cnt <= 0)
		// 		{
		// 			printf ("Client %d disconnected\n", fd);
		// 			FD_CLR(fd, &master_read_fds);
		// 			close(fd);
		// 			continue ;
		// 		}
		// 		clients[fd].request[BUF_SIZE - 1] = 0;
		// 		bzero(clients[fd].response, BUF_SIZE);
		// 		int	ret_code = parse_command(&clients[fd]);
		// 		if (ret_code == 1)
		// 		{
		// 			printf ("CLient %d disconnected\n", fd);
		// 			FD_CLR(fd, &master_read_fds);
		// 			close(fd);
		// 			continue ;
		// 		}
		// 		else
		// 		{
		// 			// printf ("buf is '%s'\n", clients[fd].response);
		// 			if (ret_code == 2 || !clients[fd].response[0])
		// 				strcpy(clients[fd].response, "Error in executing");
					
		// 			clients[fd].response[BUF_SIZE - 1] = 0;
		// 			send(fd, clients[fd].response, strlen(clients[fd].response), 0);
		// 		}
		// 	}
		// }

}