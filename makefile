# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Output executable name
TARGET = program

# Source files and headers
SRCS = commands.c process.c text.c main.c
HEADERS = commands.h process.h text.h sharedmem.h

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Compile each source file into an objective file
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
