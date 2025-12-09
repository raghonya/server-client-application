#include "server.h"

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

