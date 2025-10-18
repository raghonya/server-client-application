#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#define REQ_CAPACITY 2000
#define RESP_CHUNK 512

char	**split(char *, char);
void	free_2d_array(char **);

void communication( void )
{
	int		sockfd;
	char    request[REQ_CAPACITY];
	char    *response;
	int     n;
	char    **splitted;

	sockfd = 0;
	response = NULL;
	while (1) {
		bzero(request, sizeof(request));
		printf("Enter the command: ");
		n = 0;
		while (n < REQ_CAPACITY - 1) 
		{
			request[n] = getchar();
			if (request[n] == '\n')
				break ;	
			else if (request[n] == EOF)
			{
				close(sockfd);
				exit(0);
			}
			n++;
		}
		request[n] = 0;
		splitted = split(request, ' ');
		if (!splitted)
		{
			printf ("Allocation failure\n");
			continue ;
		}
		if (splitted[0] && strcmp(splitted[0], "connect") == 0)
		{
			struct sockaddr_in	servaddr;
			char				tmp_buf[30];

			// if (check_socket_connection(sockfd) == 0)
			if (send(sockfd, NULL, 0, MSG_NOSIGNAL) >= 0)
			{
				printf ("Already connected to the server\n");
				free_2d_array(splitted);
				continue ;
			}
			if (!splitted[1] || !splitted[2] || inet_pton(AF_INET, splitted[1], tmp_buf) == 0)
			{
				printf ("Invalid arguments\n");
				free_2d_array(splitted);
				continue ;
			}
			int	port = atoi(splitted[2]);
			// socket create and verification
			sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
			if (sockfd == -1) {
				printf("Socket creation failed\n");
				free_2d_array(splitted);
				continue ;
			}
			else printf("Socket successfully created\n");
			// fcntl(sockfd, F_SETFL, O_NONBLOCK);
			bzero(&servaddr, sizeof(servaddr));
			
			// assign IP, PORT
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = inet_addr(splitted[1]);
			servaddr.sin_port = htons(port);
			// fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);

			// connect the client socket to server socket
			if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))	< 0)
			{
				printf("Connection with the server failed\n");
				free_2d_array(splitted);
				close(sockfd);
				continue ;
			}
			else printf("Connected to the server\n");
		}
		else if (splitted[0] && \
			(strcmp(splitted[0], "disconnect") == 0 || strcmp(splitted[0], "shell") == 0))
		{
			int send_ret = send(sockfd, request, strlen(request), MSG_NOSIGNAL);
			// printf ("sendret: %d\n", send_ret);
			if (send_ret < 0)
				printf ("Youre not connected\n");
			else
			{
				response = malloc(sizeof(char) * (RESP_CHUNK + 1));
				if (!response)
				{
					printf ("Not enough memory\n");
					close(sockfd);
					continue ;
				}
				bzero(response, RESP_CHUNK + 1);
				int recv_ret = recv(sockfd, response, RESP_CHUNK, MSG_NOSIGNAL);
				if (recv_ret <= 0)
				{
					printf ("Connection lost\n");
					close(sockfd);
					continue ;
				}
				// if (recv_ret == RESP_CHUNK)
				// {
				while (recv_ret > 0)
				{
					size_t	last_len = strlen(response);
					response = realloc(response, last_len + recv_ret + 1);
					if (!response)
					{
						printf ("Malloc error\n");
						free(response);
						break ;
					}
					printf ("retval: %d\n", recv_ret);
					recv_ret = recv(sockfd, response + last_len, RESP_CHUNK, MSG_NOSIGNAL);
					printf ("after recv\n");
					if (recv_ret < 0)
					{
						printf ("Connection closed\n");
						free(response);
						break ;	
					}
					response[last_len + recv_ret] = 0;
				}
				// }
				printf("`%s`\n", response);
			}
		}
		else
			printf ("Unidentified command\n");
		free_2d_array(splitted);
	}
}

int main()
{
	communication();
}