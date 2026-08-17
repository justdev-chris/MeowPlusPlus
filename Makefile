# LLVM Configuration
LLVM_CONFIG ?= /mingw64/bin/llvm-config
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS = 

# Get LLVM flags
LLVM_CFLAGS = $(shell $(LLVM_CONFIG) --cflags 2>/dev/null || echo "")
LLVM_LIBS = $(shell $(LLVM_CONFIG) --libs --ldflags 2>/dev/null || echo "")

# Check if LLVM is available
ifneq ($(LLVM_CFLAGS),)
    HAVE_LLVM = 1
    CFLAGS += $(LLVM_CFLAGS) -DHAVE_LLVM
    LDFLAGS += $(LLVM_LIBS)
    SRCS = src/lexer.c src/parser.c src/interpreter.c src/codegen.c src/main.c
else
    HAVE_LLVM = 0
    SRCS = src/lexer.c src/parser.c src/interpreter.c src/main.c
    $(warning LLVM not found - building interpreter only)
endif

TARGET = meowplus.exe
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) *.bc *.o *.exe

.PHONY: all clean