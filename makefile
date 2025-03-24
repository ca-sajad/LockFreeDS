# Define the C compiler
CC = gcc

# Define the C flags
CFLAGS = -Wall -Wextra -I./include

# Define the source files
SRC = ./src/array.c ./test/test_array.c

# Define the object files
OBJ = $(SRC:.c=.o)

# Define the executable file
EXEC = test_array

# Default target
all: $(EXEC)

# Link the object files to create the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Compile the source files to create object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJ) $(EXEC)
