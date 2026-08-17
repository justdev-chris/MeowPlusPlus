#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meowplus.h"

// ─── GLOBALS ──────────────────────────────────────────────────
static unsigned char tape[TAPE_SIZE];
static int ptr;

// ─── HELPERS ──────────────────────────────────────────────────
static void reset_tape() {
    memset(tape, 0, TAPE_SIZE);
    ptr = 0;
}

// ─── INTERPRET MEOW++ DIRECTLY ──────────────────────────────
void interpreter_meowplus(const Token* tokens, int count) {
    reset_tape();
    
    int ip = 0;
    while (ip < count) {
        Token t = tokens[ip];
        
        switch (t.type) {
            case TOKEN_PURR:    tape[ptr]++; break;
            case TOKEN_HISS:    tape[ptr]--; break;
            case TOKEN_PAW:     ptr++; if (ptr >= TAPE_SIZE) ptr = 0; break;
            case TOKEN_PAWBACK: ptr--; if (ptr < 0) ptr = TAPE_SIZE - 1; break;
            case TOKEN_MEOW:    putchar(tape[ptr]); break;
            case TOKEN_LISTEN:  tape[ptr] = getchar(); break;
            
            case TOKEN_IFMEOW:
                if (tape[ptr] == 0) {
                    ip = t.index;
                }
                break;
                
            case TOKEN_ENDMEOW:
                if (tape[ptr] != 0) {
                    ip = t.index;
                }
                break;
                
            default: break;
        }
        ip++;
    }
}

// ─── REPL ──────────────────────────────────────────────────────
void interpreter_repl() {
    char line[256];
    printf("🐱 Meow++ REPL (type 'exit' to quit)\n");
    printf("meow++> ");
    
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, "exit") == 0) break;
        
        lexer_init(line);
        Token tokens[256];
        int count = 0;
        
        Token t = lexer_next_token();
        while (t.type != TOKEN_EOF && count < 256) {
            tokens[count++] = t;
            t = lexer_next_token();
        }
        
        if (count > 0) {
            parser_match_loops(tokens, count);
            interpreter_meowplus(tokens, count);
        }
        
        printf("\nmeow++> ");
    }
}