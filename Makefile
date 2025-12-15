# ============================= #
#           VARIABLES           #
# ============================= #

NAME        = woody-woodpacker
CC          = gcc
CFLAGS      = -Wall -Wextra -Werror
INCLUDES    = -Iincludes -Ilibft/includes

SRCS        = $(wildcard srcs/*.c)

OBJDIR      = objects
OBJS 		= objects/elf_inject.o objects/elf_reader64.o objects/main.o objects/encrypt.o objects/ft_memset.o objects/ft_memcpy.o

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
# 4. Generate header with _start offset
$(STUB_OBJ): $(STUB_SRC) stub/stub.ld
	$(CC) -Wall -Wextra -Werror -ffreestanding -fno-pie -fno-asynchronous-unwind-tables -fno-stack-protector -fcf-protection=none -c $(STUB_SRC) -o stub/stub_freestanding.o 2>/dev/null || \
	$(CC) -Wall -Wextra -Werror -ffreestanding -fno-pie -fno-asynchronous-unwind-tables -fno-stack-protector -c $(STUB_SRC) -o stub/stub_freestanding.o
	ld -T stub/stub.ld -nostdlib stub/stub_freestanding.o -o stub/stub_linked.elf
	@echo "/* Auto-generated - DO NOT EDIT */" > includes/stub_offset.h
	@echo "#define STUB_START_OFFSET 0x$$(nm stub/stub_linked.elf | grep ' _start$$' | cut -d' ' -f1)" >> includes/stub_offset.h
	@echo "#define STUB_REF_OFFSET 0x$$(nm stub/stub_linked.elf | grep ' _ref_point$$' | cut -d' ' -f1)" >> includes/stub_offset.h
	objcopy -O binary stub/stub_linked.elf $(STUB_BIN)
	ld -r -b binary -o $(STUB_OBJ) $(STUB_BIN)
	rm -f stub/stub_freestanding.o stub/stub_linked.elf

# Compile .c → objects/*.o (depends on stub for stub_offset.h)
$(OBJDIR)/%.o: srcs/%.c $(STUB_OBJ)
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
	rm -f includes/stub_offset.h

fclean: clean
	rm -f $(NAME)
	rm -f woody

re: fclean all