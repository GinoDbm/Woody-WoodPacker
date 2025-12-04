# ============================= #
#           VARIABLES           #
# ============================= #

NAME        = woody

CC          = cc
CFLAGS      = -Wall -Wextra -Werror

INCLUDES    = -Iincludes

SRCS = $(wildcard srcs/*.c)

OBJS        = $(SRCS:.c=.o)

# ============================= #
#            RULES              #
# ============================= #

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

# Compilation des .c en .o
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


# ============================= #
#           CLEANING            #
# ============================= #

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
