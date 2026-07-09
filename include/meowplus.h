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
} Token;

// ─── LEXER ──────────────────────────────────────────────────────
void lexer_init(const char* source);
Token lexer_next_token();

// ─── PARSER ────────────────────────────────────────────────────
void parser_parse(Token* tokens, int count);

// ─── INTERPRETER ──────────────────────────────────────────────
void interpreter_meowplus(const Token* tokens, int count);
void interpreter_repl();

#endif