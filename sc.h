#ifndef SC_H
# define SC_H

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
	DISCONNECT,
	ERROR
} error_codes_e;

typedef struct data_t
{
	char	*request;
	char	*response;
} data_t;

typedef enum state_e
{
	INACTIVE = 0,
	ACTIVE
} state_e;

typedef struct fds
{
	int		socket;
	state_e		state;
} fds;

char	**split(char *, char);
void	free_2d_array(char **);

#endif