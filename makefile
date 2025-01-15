CC=gcc
CFLAGS=-I./include -Wall -Wextra
SRCDIR=server/core
DBDIR=server/db
BINDIR=bin
LIBS=-lsqlite3 -lbcrypt

# Source files
SRCS=$(SRCDIR)/server.c $(SRCDIR)/router.c $(SRCDIR)/socket_pool.c $(SRCDIR)/socket.c
DB_SRCS=$(DBDIR)/user_db.c    # Make sure this file exists at server/db/user_db.c

# Object files
OBJS=$(SRCS:.c=.o)
DB_OBJS=$(DB_SRCS:.c=.o)

# Binary name
TARGET=$(BINDIR)/server

# Create bin directory if it doesn't exist
$(shell mkdir -p $(BINDIR))
$(shell mkdir -p $(DBDIR))    # Also ensure db directory exists

all: $(TARGET)

$(TARGET): $(OBJS) $(DB_OBJS)
	$(CC) $(OBJS) $(DB_OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DB_OBJS) $(TARGET)