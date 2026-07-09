#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meowplus.h"

// ─── HELPERS ──────────────────────────────────────────────────
static void print_usage() {
    printf("🐱 Meow++ - A cat-themed Brainfuck variant\n\n");
    printf("Usage:\n");
    printf("  meowplus <file.meowplus>    Run a Meow++ file\n");
    printf("  meowplus -i                 Start REPL\n");
    printf("  meowplus -h                 Show this help\n\n");
    printf("Commands:\n");
    printf("  purr     - increment\n");
    printf("  hiss     - decrement\n");
    printf("  paw      - move right\n");
    printf("  pawback  - move left\n");
    printf("  meow     - output\n");
    printf("  listen   - input\n");
    printf("  ifmeow   - loop start\n");
    printf("  endmeow  - loop end\n");
}

static char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        printf("🐾 Error: Could not open '%s'\n", path);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    
    char* buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static void run_code(const char* code) {
    lexer_init(code);
    
    Token tokens[1024];
    int count = 0;
    
    Token t = lexer_next_token();
    while (t.type != TOKEN_EOF && count < 1024) {
        tokens[count++] = t;
        t = lexer_next_token();
    }
    
    if (count == 0) {
        printf("🐾 No tokens found\n");
        return;
    }
    
    parser_parse(tokens, count);
    interpreter_meowplus(tokens, count);
    printf("\n");
}

// ─── MAIN ──────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage();
        return 0;
    }
    
    if (strcmp(argv[1], "-i") == 0) {
        interpreter_repl();
        return 0;
    }
    
    char* code = read_file(argv[1]);
    if (!code) return 1;
    
    run_code(code);
    free(code);
    return 0;
}