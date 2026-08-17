# Detect LLVM config
LLVM_CONFIG ?= /mingw64/bin/llvm-config

# Check if LLVM is available
LLVM_AVAILABLE := $(shell command -v $(LLVM_CONFIG) 2>/dev/null)

ifdef LLVM_AVAILABLE
    LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
    LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs --ldflags) -lLLVM
    USE_LLVM := 1
else
    $(warning LLVM not found, building without codegen support)
    USE_LLVM := 0
    LLVM_CFLAGS :=
    LLVM_LIBS :=
endif

CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
TARGET = meowplus.exe
SRCS = src/lexer.c src/parser.c src/interpreter.c src/main.c
OBJS = $(SRCS:.c=.o)

# Add codegen if LLVM is available
ifeq ($(USE_LLVM),1)
    SRCS += src/codegen.c
    CFLAGS += $(LLVM_CFLAGS)
    LDFLAGS = $(LLVM_LIBS)
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) *.o *.bc *.ll

.PHONY: all clean