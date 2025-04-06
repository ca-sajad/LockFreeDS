
LD = qcc
TARGET = -Vgcc_ntox86_64

CFLAGS = -Wall -Wextra $(TARGET) -I./include

# src files
LOCK_FREE_SRC = ./src/array.c
LOCK_FREE_OBJ = $(LOCK_FREE_SRC:.c=.o)

LOCK_BASED_SRC = ./test/array/lock_based_array.c
LOCK_BASED_OBJ = $(LOCK_BASED_SRC:.c=.o)

# test files
TEST_LF_REAL_SRC = ./test/array/test_lock_free_array_real.c
TEST_LB_REAL_SRC = ./test/array/test_lock_based_array_real.c

# executables
LOCK_FREE_EXEC = test_lock_free_array_real
LOCK_BASED_EXEC = test_lock_based_array_real

# build rules
all: $(LOCK_FREE_EXEC) $(LOCK_BASED_EXEC)

# 
$(LOCK_FREE_EXEC): $(LOCK_FREE_SRC:.c=.o) $(TEST_LF_REAL_SRC:.c=.o)
	$(CC) $(CFLAGS) $^ -o $@

$(LOCK_BASED_EXEC): $(LOCK_BASED_SRC:.c=.o) $(TEST_LB_REAL_SRC:.c=.o)
	$(CC) $(CFLAGS) $^ -o $@

# 
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean
clean:
	find . -name "*.o" -type f -delete
	rm -f $(LOCK_FREE_EXEC) $(LOCK_BASED_EXEC)
