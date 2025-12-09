#include "server.h"

void	*client_handler(void *cli_info)
{
	int		read_cnt;
	int		ret_code;
	cli_t	*client;
	t_data	data;

	client = (cli_t *)(cli_info);
	data = client->data;
	while (g_server_running)
	{
		data.response = NULL;
		data.request = calloc(BUF_SIZE + 1, sizeof(char));
		if (!data.request)
		{
			printf ("Malloc error\n");
			rm_client_from_list(client->socket);
			break ;
		}
		read_cnt = recv(client->socket, data.request, BUF_SIZE, MSG_NOSIGNAL);
		if  (read_cnt <= 0)
		{
			printf ("Client %d disconnected\n", client->socket);
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
		ret_code = parse_command(&data);
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
		if (!data.response)
		{
			printf ("Malloc error\n");
			rm_client_from_list(client->socket);
			return (NULL);
		}
		send(client->socket, data.response, strlen(data.response), MSG_NOSIGNAL);
		free(data.response);
	}
	return (NULL);
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
	int		index;

	index = get_client_index(client_socket);
	if (index < 0)
	{
		puts("Client socket not found");
		return (1);
	}
	if (pthread_create(&g_clients[index].handler, NULL, &client_handler, &g_clients[index]) != 0)
	{
		puts ("Pthread failed");
		rm_client_from_list(client_socket);
		return (1);
	}
	pthread_detach(g_clients[index].handler);

	return (0);
}
