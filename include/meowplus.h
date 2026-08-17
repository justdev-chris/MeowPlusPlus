#ifndef MEOWPLUS_H
#define MEOWPLUS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CODE 10000
#define TAPE_SIZE 30000

// ─── TOKEN TYPES ──────────────────────────────────────────────
typedef enum {
    TOKEN_PURR,      // +
    TOKEN_HISS,      // -
    TOKEN_PAW,       // >
    TOKEN_PAWBACK,   // <
    TOKEN_MEOW,      // .
    TOKEN_LISTEN,    // ,
    TOKEN_IFMEOW,    // [
    TOKEN_ENDMEOW,   // ]
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

// ─── TOKEN ──────────────────────────────────────────────────────
typedef struct {
    TokenType type;
    char* text;
    int line;
    int index;        // For loop matching
} Token;

// ─── LEXER ──────────────────────────────────────────────────────
void lexer_init(const char* source);
Token lexer_next_token();
void lexer_free_tokens(Token* tokens, int count);

// ─── PARSER ────────────────────────────────────────────────────
void parser_parse(Token* tokens, int count);
int parser_match_loops(Token* tokens, int count);

// ─── CODE GENERATOR ──────────────────────────────────────────
#ifdef HAVE_LLVM
void codegen_init();
void codegen_generate(const Token* tokens, int count);
void codegen_compile(const char* output_name, int optimize);
void codegen_cleanup();
#endif

// ─── INTERPRETER ──────────────────────────────────────────────
void interpreter_meowplus(const Token* tokens, int count);
void interpreter_repl();

// ─── HELPERS ──────────────────────────────────────────────────
void print_usage();
char* read_file(const char* path);
void run_code(const char* code);

#endif