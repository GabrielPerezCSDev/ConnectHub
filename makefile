CC=gcc
CFLAGS=-I./include -Wall -Wextra
SRCDIR=server/core
DBDIR=server/db
UTILDIR=server/util
BINDIR=bin
LIBS=-lsqlite3 -lbcrypt

# Source files
SRCS=$(SRCDIR)/server.c $(SRCDIR)/router.c $(SRCDIR)/socket_pool.c $(SRCDIR)/socket.c
DB_SRCS=$(DBDIR)/user_db.c    
UTIL_SRCS=$(UTILDIR)/user_cache.c

# Object files
OBJS=$(SRCS:.c=.o)
DB_OBJS=$(DB_SRCS:.c=.o)
UTIL_OBJS=$(UTIL_SRCS:.c=.o)

# Binary name
TARGET=$(BINDIR)/server

# Create bin directory if it doesn't exist
$(shell mkdir -p $(BINDIR))
$(shell mkdir -p $(DBDIR))    
$(shell mkdir -p $(UTILDIR)) 
all: $(TARGET)

$(TARGET): $(OBJS) $(DB_OBJS) $(UTIL_OBJS)
	$(CC) $(OBJS) $(DB_OBJS) $(UTIL_OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DB_OBJS) $(UTIL_OBJS) $(TARGET)