#include "server.h"

char	*receive_request(char **request, int socket)
{
	char	*tmp;
	size_t	last_len;
	int		read_cnt;

	read_cnt = BUF_SIZE;
	last_len = 0;
	do
	{
		if (*request)
			last_len = strlen(*request);
		tmp = *request;
		*request = realloc(*request, last_len + read_cnt + 1);
		if (!*request)
		{
			printf ("Malloc error\n");
			free(tmp);
			rm_client_from_list(socket);
			return (NULL);
		}
		read_cnt = recv(socket, *request + last_len, BUF_SIZE, MSG_NOSIGNAL);
		if (read_cnt < 0)
		{
			printf ("Connection closed\n");
			free(*request);
			rm_client_from_list(socket);
			return (NULL);
		}
	}
	while (strstr(*request, "\r\n") == NULL);

	return (*request);
}

int	send_response(data_t *data, int socket)
{
	int	ret_code;

	ret_code = parse_command(data);
	if (data->request) free(data->request);
	if (ret_code == DISCONNECT)
	{
		printf ("CLient %d disconnected via command\n", socket);
		send(socket, "Disconnected from the server\r\n", 30, MSG_NOSIGNAL);
		rm_client_from_list(socket);			
		return (ERROR) ;
	}
	else if (ret_code == ERROR)
		data->response = strdup("Wrong command format\r\n");
	if (!data->response)
	{
		puts("Malloc error");
		rm_client_from_list(socket);
		return (ERROR);
	}
	send(socket, data->response, strlen(data->response), MSG_NOSIGNAL);
	free(data->response);

	return (SUCCESS);
}

void	*client_handler(void *cli_info)
{
	int		ret_code;
	cli_t	*client;
	data_t	data;

	client = (cli_t *)(cli_info);
	data = client->data;
	while (g_server_running)
	{
		data.response = NULL;
		data.request = NULL;
		if (receive_request(&data.request, client->socket) == NULL)
		{
			puts("Unable to receive request: Error encountered");
			return (NULL);
		}
		if (send_response(&data, client->socket))
			return (NULL);
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
