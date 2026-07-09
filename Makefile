CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
TARGET = meowplus
SRCS = src/lexer.c src/parser.c src/interpreter.c src/main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install:
	cp $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

run: $(TARGET)
	./$(TARGET) examples/hello.meowplus

repl: $(TARGET)
	./$(TARGET) -i

help:
	./$(TARGET) -h

.PHONY: all clean install uninstall run repl help