CC=gcc
CFLAGS=-I../include -I./include -Wall -Wextra
LIBS=-lsqlite3 -lbcrypt

# Source directories
TEST_SRC=src
UNITY_SRC=include/unity
SERVER_SRC=../server

# Test source files
TEST_FILES=$(wildcard $(TEST_SRC)/*.c)
TEST_OBJS=$(TEST_FILES:.c=.o)

# Unity source
UNITY_FILES=$(UNITY_SRC)/unity.c
UNITY_OBJS=$(UNITY_FILES:.c=.o)

# Server files needed for testing
SERVER_FILES=../server/db/user_db.c 
SERVER_OBJS=$(SERVER_FILES:.c=.o)

# Test executable
TARGET=run_tests

.PHONY: all clean test

all: $(TARGET)

$(TARGET): main_test.o $(TEST_OBJS) $(UNITY_OBJS) $(SERVER_OBJS)
	$(CC) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TEST_OBJS) $(UNITY_OBJS) $(SERVER_OBJS) main_test.o $(TARGET)