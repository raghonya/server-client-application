CC				=	cc
SERVER_NAME		=	server
CLIENT_NAME		=	client
CFLAGS			=	-pthread -g -Wall -Wextra# -Werror
# CFLAGS			+=	-g -fsanitize=address
HEADERS			=	sc.h
DEP				=	Makefile $(HEADERS)
OBJDIR			=	obj
SERVER_SRCS		=	server.c split.c utils.c
CLIENT_SRCS		=	client.c split.c utils.c
SERVER_OBJS		=	$(SERVER_SRCS:%.c=$(OBJDIR)/%.o)
CLIENT_OBJS		=	$(CLIENT_SRCS:%.c=$(OBJDIR)/%.o)

CMD		=	$(MAKECMDGOALS)

all: objdir $(SERVER_NAME) $(CLIENT_NAME)

objdir:
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c $(DEP)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(SERVER_NAME): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(CLIENT_NAME): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(SERVER_NAME)
	rm -f $(CLIENT_NAME)

re:	fclean all

.PHONY: all clean fclean re objdir