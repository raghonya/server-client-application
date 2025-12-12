#include "server.h"

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

int	create_server(struct sockaddr_in *servaddr, int port)
{
	int	opt;
	int	server_socket;

	opt = 0;
	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr->sin_port = htons(port);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		printf("socket creation failed\n");
		exit(1);
	}
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if ((bind(server_socket, (struct sockaddr *)servaddr, sizeof(*servaddr))) != 0)
	{
		printf("socket bind failed\n");
		exit(1);
	}
	else
		printf("Socket successfully binded\n");
	if ((listen(server_socket, MAX_CLI_COUNT)) != 0)
	{
		printf("Listen failed\n");
		exit(1);
	}

	return (server_socket);
}

void	init_variables(struct sigaction *act, struct sockaddr_in *servaddr, char *port)
{
	int	i_port;

	i_port = atoi(port);
	set_signal_action(act);
	bzero(g_clients, sizeof(g_clients));
	pthread_mutex_init(&g_cli_cnt_mutex, NULL);
	bzero(servaddr, sizeof(*servaddr));

	g_server_socket = create_server(servaddr, i_port);
}

int main(int argc, char **argv)
{
	struct sigaction	act;
	struct sockaddr_in	servaddr;
	int					client_socket;

	if (argc < 2 || argc > 3)
	{
		puts("Usage: ./server 'Port of the server'");
		return (1);
	}
	init_variables(&act, &servaddr, argv[1]);
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
