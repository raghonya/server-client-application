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

#define PORT			7744
#define BUF_SIZE		100
#define READ_SIZE		200

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


int	run_shell_command(char *full_command, char (*response)[BUF_SIZE])
{
	FILE	*fp;

	fp = popen(full_command, "r");
	if (!fp)
		return (2);
	while (fgets(*response + strlen(*response), BUF_SIZE, fp))
		;
	return (0);
}

int	parse_command(data_t *client)
{
	int		ret_code;
	char	**splitted;

	ret_code = 0;
	splitted = split(client->request, ' ');
	if (!splitted)
		return (2);
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
				ret_code = 1;
			else
			{
				client->request[i] = 0;
				// printf ("sedning to execute '%s'\n", client->request + from_start + 1);
				run_shell_command(client->request + from_start + 1, &client->response);
			}
		}
	}
	free_2d_array(splitted);
	return (ret_code);
}

void	*client_handler(void *p_socket)
{
	int		socket = *(int *)(p_socket);
	int		read_cnt;
	data_t	data;

	data.request = malloc(sizeof(char) * (BUF_SIZE + 1));
	while ((read_cnt = recv(socket, data.request, BUF_SIZE, MSG_NOSIGNAL)))
	{

	}
}

int main()
{
	int					server_socket;
	struct sockaddr_in	servaddr;

	struct sockaddr_in	cli;
	socklen_t			cli_len;
	data_t				clients[5];
	int					client_socket;
	pthread_mutex_t		client_count_mutex;
	pthread_t			client_thread;
	int					client_count;

	client_count = 0;
	cli_len = sizeof(cli);
	pthread_mutex_init(&client_count_mutex, NULL);

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
	while ((client_socket = accept(server_socket, (struct sockaddr *)&cli, &cli_len)))
	{

		if (client_socket < 0)
		{
			printf ("Accept error\n");
			continue ;
		}
		pthread_mutex_lock(&client_count_mutex);
		if (client_count > 5)
		{
			printf ("Server supports only 5 clients at once, try connecting later\n");
			send(client_socket, "Busy", 4, MSG_NOSIGNAL);
			clsoe(client_socket);
			pthread_mutex_unlock(&client_count_mutex);
			continue ;
		}
		client_count++;
		pthread_mutex_unlock(&client_count_mutex);

		int		*new_client_socket = malloc(sizeof(int));
		if (!new_client_socket)
		{
			printf ("Malloc error\n");
			close(client_socket);
			continue ;
		}
		pthread_create(&client_thread, NULL, &client_handler, &new_client_socket);
		pthread_detach(&client_thread);
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

}