# ============================= #
#           VARIABLES           #
# ============================= #

NAME        = woody
CC          = cc
CFLAGS      = -Wall -Wextra -Werror
INCLUDES    = -Iincludes

SRCS        = $(wildcard srcs/*.c)
OBJS        = $(SRCS:.c=.o)

STUB_SRC    = stub/stub.c
STUB_OBJ    = stub/stub.o
STUB_BIN    = stub/stub.bin   # binaire brut temporaire

# ============================= #
#            RULES              #
# ============================= #

all: $(STUB_OBJ) $(NAME)

# Build woody with stub object linked
$(NAME): $(OBJS) $(STUB_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(STUB_OBJ) -o $(NAME)

# Compile stub.c → stub.o via binaire intermédiaire
# Étapes : 1) .o freestanding, 2) binaire brut, 3) .o final avec ld -b binary
$(STUB_OBJ): $(STUB_SRC)
	$(CC) -Wall -Wextra -Werror -ffreestanding -fno-pie -c $(STUB_SRC) -o stub/stub_freestanding.o
	objcopy -O binary stub/stub_freestanding.o $(STUB_BIN)
	ld -r -b binary -o $(STUB_OBJ) $(STUB_BIN)
	rm -f stub/stub_freestanding.o

# Build all .c → .o (Woody sources)
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ============================= #
#           CLEANING            #
# ============================= #

clean:
	rm -f $(OBJS) $(STUB_OBJ) $(STUB_BIN)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
