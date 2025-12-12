#ifndef SERVER_H
# define SERVER_H

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socketvar.h>

#define PORT			7747
#define BUF_SIZE		128
#define TIMEOUT_SEC		3
#define MAX_CLI_COUNT	5

#define INCREASE		1
#define DECREASE		0

#define FAILED(cause)	"Error: "cause"\r\n"

typedef enum error_codes_e
{
	SUCCESS = 0,
	ERROR,
	DISCONNECT
} error_codes_e;

typedef struct s_data
{
	char	*request;
	char	*response;
} data_t;

typedef enum s_state
{
	INACTIVE = 0,
	ACTIVE
} e_state;

typedef struct cli_t
{
	int			socket;
	pthread_t	handler;
	e_state		state;
	data_t		data;
} cli_t;

extern int				g_server_socket;
extern int volatile		g_server_running;
extern int	volatile	g_active_clients;
extern pthread_mutex_t	g_cli_cnt_mutex;
extern cli_t			g_clients[MAX_CLI_COUNT];

char	**split(char *, char);
void	free_2d_array(char **);
void	change_clients_count(pthread_mutex_t *mut, int side);
int		get_client_index(int socket);
void	rm_client_from_list(int socket);
void	add_client_to_list(int socket);

int		accept_client();
int		create_client_handler(int client_socket);
char	*create_response(int pipefd);
char	*run_shell_command(char *full_command);
int		validate_command(char *cmd, int *start_index);
int		parse_command(data_t *client);

#endif