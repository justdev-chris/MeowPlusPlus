#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "meowplus.h"

// ─── HELPERS ──────────────────────────────────────────────────
void print_usage() {
    printf("🐱 Meow++ LLVM Compiler & Interpreter\n\n");
    printf("Usage:\n");
    printf("  meowplus <file.meowplus> [options]    Compile to executable\n");
    printf("  meowplus -i                           Start REPL\n");
    printf("  meowplus -r <file.meowplus>           Run with interpreter\n");
    printf("  meowplus -h                           Show this help\n\n");
    printf("Options:\n");
    printf("  -o <output>    Output filename (default: a.out)\n");
    printf("  -O<level>      Optimization level 0-3 (default: 2)\n");
    printf("  -S             Generate LLVM IR instead of executable\n\n");
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

char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "🐾 Error: Could not open '%s'\n", path);
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
    
    size_t read = fread(buf, 1, size, f);
    buf[read] = '\0';
    fclose(f);
    return buf;
}

void run_code(const char* code) {
    lexer_init(code);
    
    Token tokens[1024];
    int count = 0;
    
    Token t = lexer_next_token();
    while (t.type != TOKEN_EOF && count < 1024) {
        tokens[count++] = t;
        t = lexer_next_token();
    }
    
    if (count == 0) {
        fprintf(stderr, "🐾 No tokens found\n");
        return;
    }
    
    parser_match_loops(tokens, count);
    interpreter_meowplus(tokens, count);
    printf("\n");
    lexer_free_tokens(tokens, count);
}

static int parse_optimization(const char* arg) {
    if (strncmp(arg, "-O", 2) == 0 && strlen(arg) == 3) {
        int level = arg[2] - '0';
        if (level >= 0 && level <= 3) return level;
    }
    return 2; // Default optimization level
}

// ─── MAIN ──────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    // Help
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage();
        return 0;
    }
    
    // REPL
    if (strcmp(argv[1], "-i") == 0) {
        interpreter_repl();
        return 0;
    }
    
    // Run with interpreter
    if (strcmp(argv[1], "-r") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No input file specified\n");
            return 1;
        }
        char* code = read_file(argv[2]);
        if (!code) return 1;
        run_code(code);
        free(code);
        return 0;
    }
    
    // Compilation mode
    char* input_file = argv[1];
    char* output_file = "a.out";
    int optimize = 2;
    int emit_ir = 0;
    
    // Parse options
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strncmp(argv[i], "-O", 2) == 0) {
            optimize = parse_optimization(argv[i]);
        } else if (strcmp(argv[i], "-S") == 0) {
            emit_ir = 1;
        }
    }
    
    char* code = read_file(input_file);
    if (!code) return 1;
    
    // Tokenize
    lexer_init(code);
    Token tokens[1024];
    int count = 0;
    Token t = lexer_next_token();
    while (t.type != TOKEN_EOF && count < 1024) {
        tokens[count++] = t;
        t = lexer_next_token();
    }
    
    if (count == 0) {
        fprintf(stderr, "🐾 No tokens found\n");
        free(code);
        return 1;
    }
    
    // Parse and match loops
    parser_match_loops(tokens, count);
    
    // Check if LLVM is available (codegen functions exist)
    #ifdef HAVE_LLVM
        if (emit_ir) {
            // Generate LLVM IR
            codegen_init();
            codegen_generate(tokens, count);
            
            char* ir_file = malloc(strlen(output_file) + 4);
            sprintf(ir_file, "%s.ll", output_file);
            
            char* error = NULL;
            if (LLVMPrintModuleToFile(module, ir_file, &error) != 0) {
                fprintf(stderr, "Failed to write IR: %s\n", error);
                free(ir_file);
            } else {
                printf("✅ Generated LLVM IR to %s\n", ir_file);
                free(ir_file);
            }
            
            codegen_cleanup();
        } else {
            // Compile to executable
            codegen_init();
            codegen_generate(tokens, count);
            codegen_compile(output_file, optimize);
            codegen_cleanup();
        }
    #else
        // No LLVM available - just run with interpreter
        fprintf(stderr, "⚠️ LLVM not available - running with interpreter instead\n");
        interpreter_meowplus(tokens, count);
        printf("\n");
    #endif
    
    lexer_free_tokens(tokens, count);
    free(code);
    return 0;
}