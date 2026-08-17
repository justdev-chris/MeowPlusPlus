CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LLVM_CFLAGS = $(shell llvm-config --cflags)
LLVM_LIBS = $(shell llvm-config --libs --ldflags) -lLLVM

TARGET = meowplus
SRCS = src/lexer.c src/parser.c src/interpreter.c src/codegen.c src/main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LLVM_CFLAGS) -o $(TARGET) $(OBJS) $(LLVM_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(LLVM_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) *.o *.bc *.ll

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

run: $(TARGET)
	./$(TARGET) -r examples/hello.meowplus

repl: $(TARGET)
	./$(TARGET) -i

help:
	./$(TARGET) -h

# Example with compilation
example: $(TARGET)
	./$(TARGET) examples/hello.meowplus -o hello -O2
	./hello

# Generate LLVM IR
ir: $(TARGET)
	./$(TARGET) examples/hello.meowplus -o hello -S

.PHONY: all clean install uninstall run repl help example ir