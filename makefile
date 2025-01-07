CC=gcc
CFLAGS=-I./include -Wall -Wextra
SRCDIR=server/core
BINDIR=bin

# Source files
SRCS=$(SRCDIR)/server.c $(SRCDIR)/router.c $(SRCDIR)/socket_pool.c $(SRCDIR)/socket.c

# Object files
OBJS=$(SRCS:.c=.o)

# Binary name
TARGET=$(BINDIR)/server

# Create bin directory if it doesn't exist
$(shell mkdir -p $(BINDIR))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)