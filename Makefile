# ============================= #
#           VARIABLES           #
# ============================= #

NAME        = woody-woodpacker
CC          = cc
CFLAGS      = -Wall -Wextra -Werror
INCLUDES    = -Iincludes -Ilibft/includes

SRCS        = $(wildcard srcs/*.c)

OBJDIR      = objects
OBJS = objects/elf_inject.o objects/elf_reader64.o objects/main.o objects/ft_memset.o objects/ft_memcpy.o

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
# 1. Compile to object file with freestanding flags
# 2. Link with custom linker script to resolve RIP-relative relocations
# 3. Extract binary from the linked ELF
$(STUB_OBJ): $(STUB_SRC) stub/stub.ld
	$(CC) -Wall -Wextra -Werror -ffreestanding -fno-pie -fno-asynchronous-unwind-tables -fno-stack-protector -c $(STUB_SRC) -o stub/stub_freestanding.o
	ld -T stub/stub.ld -nostdlib stub/stub_freestanding.o -o stub/stub_linked.elf
	objcopy -O binary stub/stub_linked.elf $(STUB_BIN)
	ld -r -b binary -o $(STUB_OBJ) $(STUB_BIN)
	rm -f stub/stub_freestanding.o stub/stub_linked.elf

# Compile .c → objects/*.o
$(OBJDIR)/%.o: srcs/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile libft .c → objects/*.o
$(OBJDIR)/%.o: libft/srcs/%.c
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