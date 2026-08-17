#include <stdio.h>
#include <stdlib.h>
#include "meowplus.h"

// ─── GLOBALS ──────────────────────────────────────────────────
static Token current_token;
static Token* all_tokens;
static int token_count;
static int pos;

// ─── HELPERS ──────────────────────────────────────────────────
static void advance() {
    if (pos < token_count) {
        current_token = all_tokens[pos++];
    }
}

static int match(TokenType type) {
    if (current_token.type == type) {
        advance();
        return 1;
    }
    return 0;
}

static void expect(TokenType type, const char* msg) {
    if (current_token.type != type) {
        fprintf(stderr, "⚠️ Expected '%s' at line %d\n", msg, current_token.line);
        exit(1);
    }
    advance();
}

// ─── PARSE ─────────────────────────────────────────────────────
void parser_parse(Token* tokens, int count) {
    all_tokens = tokens;
    token_count = count;
    pos = 0;
    
    if (count == 0) {
        fprintf(stderr, "⚠️ No tokens to parse\n");
        return;
    }
    
    advance();
    
    // Validate tokens
    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOKEN_ERROR) {
            fprintf(stderr, "⚠️ Invalid tokens found. Aborting.\n");
            exit(1);
        }
    }
    
    // Match loops
    if (!parser_match_loops(tokens, count)) {
        fprintf(stderr, "⚠️ Unmatched loops found\n");
        exit(1);
    }
}

// ─── MATCH LOOPS ─────────────────────────────────────────────
int parser_match_loops(Token* tokens, int count) {
    int stack[1024];
    int stack_size = 0;
    
    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOKEN_IFMEOW) {
            stack[stack_size++] = i;
        } else if (tokens[i].type == TOKEN_ENDMEOW) {
            if (stack_size == 0) {
                fprintf(stderr, "⚠️ Unmatched 'endmeow' at line %d\n", tokens[i].line);
                return 0;
            }
            int start = stack[--stack_size];
            tokens[start].index = i;
            tokens[i].index = start;
        }
    }
    
    if (stack_size > 0) {
        fprintf(stderr, "⚠️ Unmatched 'ifmeow' at line %d\n", tokens[stack[stack_size-1]].line);
        return 0;
    }
    
    return 1;
}