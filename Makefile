# ============================= #
#           VARIABLES           #
# ============================= #

NAME        = woody-woodpacker
CC          = cc
CFLAGS      = -Wall -Wextra -Werror
INCLUDES    = -Iincludes

SRCS        = $(wildcard srcs/*.c)

OBJDIR      = objects
OBJS = objects/elf_inject.o objects/elf_reader64.o objects/main.o

STUB_SRC    = stub/stub.c
STUB_OBJ    = stub/stub.o
STUB_BIN    = stub/stub.bin

# ============================= #
#            RULES              #
# ============================= #

all: $(OBJDIR) $(STUB_OBJ) $(NAME)

$(OBJDIR):
	mkdir -p $(OBJDIR)

# Link final packer
$(NAME): $(OBJS) $(STUB_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(STUB_OBJ) -o $(NAME)

# Compile stub with binary embedding
$(STUB_OBJ): $(STUB_SRC)
	$(CC) -Wall -Wextra -Werror -ffreestanding -fno-pie -c $(STUB_SRC) -o stub/stub_freestanding.o
	objcopy -O binary stub/stub_freestanding.o $(STUB_BIN)
	ld -r -b binary -o $(STUB_OBJ) $(STUB_BIN)
	rm -f stub/stub_freestanding.o

# Compile .c → objects/*.o
$(OBJDIR)/%.o: srcs/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ============================= #
#           CLEANING            #
# ============================= #

clean:
	rm -f $(OBJS) $(STUB_OBJ) $(STUB_BIN)
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)
	rm -f woody

re: fclean all

.PHONY: all clean fclean re