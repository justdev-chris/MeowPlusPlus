#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "meowplus.h"

// ─── GLOBALS ──────────────────────────────────────────────────
static const char* source;
static int pos;
static int line;

// ─── KEYWORD MAP ──────────────────────────────────────────────
typedef struct {
    const char* word;
    TokenType type;
} Keyword;

static Keyword keywords[] = {
    {"purr", TOKEN_PURR},
    {"hiss", TOKEN_HISS},
    {"paw", TOKEN_PAW},
    {"pawback", TOKEN_PAWBACK},
    {"meow", TOKEN_MEOW},
    {"listen", TOKEN_LISTEN},
    {"ifmeow", TOKEN_IFMEOW},
    {"endmeow", TOKEN_ENDMEOW},
    {NULL, TOKEN_ERROR}
};

// ─── INIT ──────────────────────────────────────────────────────
void lexer_init(const char* src) {
    source = src;
    pos = 0;
    line = 1;
}

// ─── HELPERS ──────────────────────────────────────────────────
static char peek() {
    return source[pos];
}

static char advance() {
    return source[pos++];
}

static int is_at_end() {
    return source[pos] == '\0';
}

static void skip_whitespace() {
    while (!is_at_end() && isspace(peek())) {
        if (peek() == '\n') line++;
        advance();
    }
}

// ─── SCAN KEYWORD ─────────────────────────────────────────────
static TokenType scan_keyword(const char* word) {
    for (int i = 0; keywords[i].word != NULL; i++) {
        if (strcmp(word, keywords[i].word) == 0)
            return keywords[i].type;
    }
    return TOKEN_ERROR;
}

// ─── NEXT TOKEN ───────────────────────────────────────────────
Token lexer_next_token() {
    skip_whitespace();
    
    Token token;
    token.line = line;
    token.text = NULL;
    
    if (is_at_end()) {
        token.type = TOKEN_EOF;
        return token;
    }
    
    // Skip comments
    if (peek() == '#') {
        while (!is_at_end() && peek() != '\n') advance();
        return lexer_next_token();
    }
    
    // Skip punctuation (commas, periods, etc.)
    if (ispunct(peek()) && peek() != '#') {
        advance();
        return lexer_next_token();
    }
    
    // Read word (letters, digits, underscores)
    if (isalpha(peek()) || peek() == '_') {
        char word[64];
        int i = 0;
        while (!is_at_end() && (isalnum(peek()) || peek() == '_') && peek() != '\n' && peek() != '\r') {
            word[i++] = advance();
        }
        word[i] = '\0';
        
        token.type = scan_keyword(word);
        if (token.type == TOKEN_ERROR) {
            printf("⚠️ Unknown word: '%s' at line %d\n", word, line);
        }
        token.text = strdup(word);
        return token;
    }
    
    // Unexpected character
    printf("⚠️ Unexpected character '%c' at line %d\n", peek(), line);
    advance();
    token.type = TOKEN_ERROR;
    return token;
}