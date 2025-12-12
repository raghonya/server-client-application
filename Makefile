CC				=	cc
CFLAGS			=	-pthread -g #-Wall -Wextra# -Werror
CFLAGS			+=	-g -fsanitize=address
# CFLAGS			+=	-g -fsanitize=undefined
OBJDIR			=	obj
BIN				=	bin

# Server variables
SERVER_NAME		=	server
SERVER_INC_DIR	=	includes_server
SERVER_SRC_DIR	=	src_server
SERVER_INCS		=	$(wildcard $(SERVER_INC_DIR)/*.h $(SERVER_INC_DIR)/*.hpp)
SERVER_IFLAGS	=	-I $(SERVER_INC_DIR)
SERVER_DEP		=	Makefile $(SERVER_INCs)
SERVER_SRCS		=	$(wildcard $(SERVER_SRC_DIR)/*.c)
# SERVER_SRCS		=	$(shell find $(SERVER_SRC_DIR) -name "*.c" -exec basename \{} .po \;)
SERVER_OBJS		=	$(SERVER_SRCS:%.c=$(OBJDIR)/%.o)

# Client variables
CLIENT_NAME		=	client
CLIENT_INC_DIR	=	includes_client
CLIENT_SRC_DIR	=	src_client
CLIENT_INCS		=	$(wildcard $(CLIENT_INC_DIR)/*.h $(CLIENT_INC_DIR)/*.hpp)
CLIENT_IFLAGS	=	-I $(CLIENT_INC_DIR)
CLIENT_DEP		=	Makefile $(CLIENT_INCS)
CLIENT_SRCS		=	$(wildcard $(CLIENT_SRC_DIR)/*.c)
# CLIENT_SRCS		=	$(shell find $(CLIENT_SRC_DIR) -name "*.c" -exec basename \{} .po \;)
CLIENT_OBJS		=	$(CLIENT_SRCS:%.c=$(OBJDIR)/%.o)

all: objdir $(BIN)/$(SERVER_NAME) $(BIN)/$(CLIENT_NAME)

objdir:
	@mkdir -p $(BIN)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(OBJDIR)/$(SERVER_SRC_DIR)
	@mkdir -p $(OBJDIR)/$(CLIENT_SRC_DIR)

$(OBJDIR)/$(SERVER_SRC_DIR)/%.o: $(SERVER_SRC_DIR)/%.c $(SERVER_DEP)
	$(CC) $(CFLAGS) $(SERVER_IFLAGS) -c $< -o $@

$(OBJDIR)/$(CLIENT_SRC_DIR)/%.o: $(CLIENT_SRC_DIR)/%.c $(CLIENT_DEP)
	$(CC) $(CFLAGS) $(CLIENT_IFLAGS) -c $< -o $@

# $(OBJDIR)/%.o: %.c $(SERVER_DEP)
# 	$(CC) $(CFLAGS) $(SERVER_IFLAGS) -c $< -o $@

# $(OBJDIR)/%.o: %.c $(CLIENT_DEP)
# 	$(CC) $(CFLAGS) $(CLIENT_IFLAGS) -c $< -o $@

$(BIN)/$(SERVER_NAME): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN)/$(CLIENT_NAME): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -rf $(BIN)

re:	fclean all

.PHONY: all objdir clean fclean re