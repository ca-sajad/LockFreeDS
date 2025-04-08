
#LD = qcc
#TARGET = -Vgcc_ntox86_64
#CFLAGS = -Wall -Wextra $(TARGET) -I./include

LD = gcc
CFLAGS = -Wall -Wextra -I./include

# array src files
LOCK_FREE_ARRAY_SRC = ./src/array.c
LOCK_FREE_ARRAY_OBJ = $(LOCK_FREE_ARRAY_SRC:.c=.o)

LOCK_BASED_ARRAY_SRC = ./test/array/lock_based_array.c
LOCK_BASED_ARRAY_OBJ = $(LOCK_BASED_ARRAY_SRC:.c=.o)

# array test files
TEST_LF_REAL_ARRAY_SRC = ./test/array/test_lock_free_array_real.c
TEST_LB_REAL_ARRAY_SRC = ./test/array/test_lock_based_array_real.c

# array executables
LOCK_FREE_ARRAY_EXEC = test_lock_free_array_real
LOCK_BASED_ARRAY_EXEC = test_lock_based_array_real

# queue src files
LOCK_FREE_QUEUE_SRC = ./src/queue.c
LOCK_FREE_QUEUE_OBJ = $(LOCK_FREE_QUEUE_SRC:.c=.o)

LOCK_BASED_QUEUE_SRC = ./test/queue/lock_based_queue.c
LOCK_BASED_QUEUE_OBJ = $(LOCK_BASED_QUEUE_SRC:.c=.o)

# queue test files
TEST_LF_QUEUE_SRC = ./test/queue/test_lock_free_queue.c
TEST_LB_QUEUE_SRC = ./test/queue/test_lock_based_queue.c

# queue executables
LOCK_FREE_QUEUE_EXEC = test_lock_free_queue
LOCK_BASED_QUEUE_EXEC = test_lock_based_queue

# build rules
all: $(LOCK_FREE_ARRAY_EXEC) $(LOCK_BASED_ARRAY_EXEC) $(LOCK_FREE_QUEUE_EXEC) $(LOCK_BASED_QUEUE_EXEC)

# 
$(LOCK_FREE_ARRAY_EXEC): $(LOCK_FREE_ARRAY_SRC:.c=.o) $(TEST_LF_REAL_ARRAY_SRC:.c=.o)
	$(CC) $(CFLAGS) $^ -o $@

$(LOCK_BASED_ARRAY_EXEC): $(LOCK_BASED_ARRAY_SRC:.c=.o) $(TEST_LB_REAL_ARRAY_SRC:.c=.o)
	$(CC) $(CFLAGS) $^ -o $@

$(LOCK_FREE_QUEUE_EXEC): $(LOCK_FREE_QUEUE_SRC:.c=.o) $(TEST_LF_QUEUE_SRC:.c=.o)
	$(CC) $(CFLAGS) $^ -o $@

$(LOCK_BASED_QUEUE_EXEC): $(LOCK_BASED_QUEUE_SRC:.c=.o) $(TEST_LB_QUEUE_SRC:.c=.o)
	$(CC) $(CFLAGS) $^ -o $@

# 
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean
clean:
	find . -name "*.o" -type f -delete
	rm -f $(LOCK_FREE_ARRAY_EXEC) $(LOCK_BASED_ARRAY_EXEC) $(LOCK_FREE_QUEUE_EXEC) $(LOCK_BASED_QUEUE_EXEC)
