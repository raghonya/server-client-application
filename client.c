#include "sc.h"

#define REQ_CAPACITY	2048
#define RESP_CHUNK		512
#define DELIM			"\r\n\t\f\v "

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
		while (n < REQ_CAPACITY - 1 - 3) 
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
		request[n] = ' ';
		request[n + 1] = '\r';
		request[n + 2] = '\n';
		request[n + 3] = 0;
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
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd == -1) {
				printf("Socket creation failed\n");
				free_2d_array(splitted);
				continue ;
			}
			else printf("Socket successfully created\n");
			bzero(&servaddr, sizeof(servaddr));
			
			// assign IP, PORT
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = inet_addr(splitted[1]);
			// servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
			servaddr.sin_port = htons(port);
			
			// splitted[1] = malloc(1);

			// fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);

			// connect the client socket to server socket
			if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))	< 0)
			{
				printf("Connection with the server failed\n");
				free_2d_array(splitted);
				close(sockfd);
				continue ;
			}
			// else printf("Connected to the server\n");
		}
		else if (splitted[0] && \
			(strcmp(splitted[0], "disconnect") == 0 || strcmp(splitted[0], "shell") == 0))
		{
			if (send(sockfd, NULL, 0, MSG_NOSIGNAL) < 0)
			{
				printf ("Not connected to the server\n");
				free_2d_array(splitted);
				continue ;
			}
			int send_ret = send(sockfd, request, strlen(request), MSG_NOSIGNAL);
			if (send_ret < 0)
				printf ("Youre not connected\n");
			else
			{
				char	tmp_buf[RESP_CHUNK + 1];
				int		recv_ret = 1;

				response = malloc(sizeof(char));
				if (!response)
				{
					printf ("Not enough memory\n");
					close(sockfd);
					continue ;
				}
				*response = 0;
				while (strstr(response, "\r\n") == NULL)
				{
					// printf ("resp: '%s'\n", response);
					bzero(tmp_buf, RESP_CHUNK + 1);
					// printf ("mors aziz arev\n");
					recv_ret = recv(sockfd, tmp_buf, RESP_CHUNK, MSG_NOSIGNAL);
					// printf ("stees aper\n");
					if (recv_ret < 0)
					{
						printf ("Connection lost\n");
						free(response);
						close(sockfd);
						break ;
					}
					size_t	last_len = strlen(response);
					char	*tmp = response;
					response = realloc(response, last_len + recv_ret + 1);
					if (!response)
					{
						printf ("Not enough memory\n");
						free(tmp);
						close(sockfd);
						break ;
					}
					strcat(response, tmp_buf);
				}
				if (strcmp(splitted[0], "disconnect") == 0 || strcmp(response, "Server is closed\r\n") == 0)
					close(sockfd);
				char *tmp = strstr(response, "\r\n");
				response[tmp - response] = 0;
				printf("%s\n", response);
				free(response);
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