#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include "meowplus.h"

// ─── GLOBALS ──────────────────────────────────────────────────
static LLVMModuleRef module;
static LLVMBuilderRef builder;
static LLVMValueRef tape_ptr;
static LLVMValueRef ptr_var;
static LLVMValueRef current_func;
static LLVMBasicBlockRef current_bb;
static LLVMContextRef context;

// ─── LOOP STACK ──────────────────────────────────────────────
typedef struct {
    LLVMBasicBlockRef condition;
    LLVMBasicBlockRef body;
    LLVMBasicBlockRef end;
} LoopInfo;

static LoopInfo loop_stack[1024];
static int loop_stack_size = 0;

// ─── INIT ──────────────────────────────────────────────────────
void codegen_init() {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    
    context = LLVMContextCreate();
    module = LLVMModuleCreateWithNameInContext("meowplus", context);
    
    // Set target triple
    LLVMSetTarget(module, LLVMGetDefaultTargetTriple());
    
    builder = LLVMCreateBuilderInContext(context);
    
    // Create tape array (30000 bytes)
    LLVMTypeRef tape_type = LLVMArrayType(LLVMInt8TypeInContext(context), TAPE_SIZE);
    tape_ptr = LLVMAddGlobal(module, tape_type, "tape");
    LLVMSetLinkage(tape_ptr, LLVMPrivateLinkage);
    LLVMSetInitializer(tape_ptr, LLVMConstNull(tape_type));
    
    // Create pointer variable (i32)
    LLVMTypeRef ptr_type = LLVMInt32TypeInContext(context);
    ptr_var = LLVMAddGlobal(module, ptr_type, "ptr");
    LLVMSetLinkage(ptr_var, LLVMPrivateLinkage);
    LLVMSetInitializer(ptr_var, LLVMConstInt(ptr_type, 0, 0));
    
    // Add external functions
    LLVMTypeRef putchar_args[] = { LLVMInt32TypeInContext(context) };
    LLVMTypeRef putchar_type = LLVMFunctionType(LLVMInt32TypeInContext(context), putchar_args, 1, 0);
    LLVMAddFunction(module, "putchar", putchar_type);
    
    LLVMTypeRef getchar_type = LLVMFunctionType(LLVMInt32TypeInContext(context), NULL, 0, 0);
    LLVMAddFunction(module, "getchar", getchar_type);
}

// ─── EMIT PUTCHAR ─────────────────────────────────────────────
static void emit_putchar(LLVMValueRef val) {
    LLVMValueRef putchar = LLVMGetNamedFunction(module, "putchar");
    LLVMValueRef int_arg = LLVMBuildZExt(builder, val, LLVMInt32TypeInContext(context), "");
    LLVMBuildCall2(builder, LLVMInt32TypeInContext(context), putchar, &int_arg, 1, "");
}

// ─── EMIT GETCHAR ─────────────────────────────────────────────
static void emit_getchar(LLVMValueRef cell_ptr) {
    LLVMValueRef getchar = LLVMGetNamedFunction(module, "getchar");
    LLVMValueRef result = LLVMBuildCall2(builder, LLVMInt32TypeInContext(context), getchar, NULL, 0, "");
    result = LLVMBuildTrunc(builder, result, LLVMInt8TypeInContext(context), "");
    LLVMBuildStore(builder, result, cell_ptr);
}

// ─── GET CELL POINTER ─────────────────────────────────────────
static LLVMValueRef get_cell_ptr(LLVMValueRef ptr_val) {
    LLVMValueRef tape = LLVMBuildLoad2(builder, LLVMArrayType(LLVMInt8TypeInContext(context), TAPE_SIZE), tape_ptr, "tape_load");
    LLVMValueRef indices[] = {
        LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0),
        ptr_val
    };
    return LLVMBuildGEP2(builder, LLVMArrayType(LLVMInt8TypeInContext(context), TAPE_SIZE), tape, indices, 2, "cell_ptr");
}

// ─── GENERATE MAIN FUNCTION ──────────────────────────────────
LLVMValueRef codegen_create_main() {
    LLVMTypeRef main_type = LLVMFunctionType(LLVMVoidTypeInContext(context), NULL, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", main_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, main_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    
    current_func = main_func;
    current_bb = entry;
    
    return main_func;
}

// ─── GENERATE CODE ────────────────────────────────────────────
void codegen_generate(const Token* tokens, int count) {
    LLVMValueRef main_func = codegen_create_main();
    if (!main_func) {
        fprintf(stderr, "Failed to create main function\n");
        return;
    }
    
    // Initialize ptr
    LLVMValueRef ptr = LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0);
    LLVMBuildStore(builder, ptr, ptr_var);
    
    // Get initial cell pointer
    LLVMValueRef cell_ptr = get_cell_ptr(ptr);
    
    // Process tokens
    for (int i = 0; i < count; i++) {
        switch (tokens[i].type) {
            case TOKEN_PURR: {
                LLVMValueRef val = LLVMBuildLoad2(builder, LLVMInt8TypeInContext(context), cell_ptr, "");
                val = LLVMBuildAdd(builder, val, LLVMConstInt(LLVMInt8TypeInContext(context), 1, 0), "");
                LLVMBuildStore(builder, val, cell_ptr);
                break;
            }
            case TOKEN_HISS: {
                LLVMValueRef val = LLVMBuildLoad2(builder, LLVMInt8TypeInContext(context), cell_ptr, "");
                val = LLVMBuildSub(builder, val, LLVMConstInt(LLVMInt8TypeInContext(context), 1, 0), "");
                LLVMBuildStore(builder, val, cell_ptr);
                break;
            }
            case TOKEN_PAW: {
                ptr = LLVMBuildAdd(builder, ptr, LLVMConstInt(LLVMInt32TypeInContext(context), 1, 0), "");
                LLVMBuildStore(builder, ptr, ptr_var);
                cell_ptr = get_cell_ptr(ptr);
                break;
            }
            case TOKEN_PAWBACK: {
                ptr = LLVMBuildSub(builder, ptr, LLVMConstInt(LLVMInt32TypeInContext(context), 1, 0), "");
                LLVMBuildStore(builder, ptr, ptr_var);
                cell_ptr = get_cell_ptr(ptr);
                break;
            }
            case TOKEN_MEOW: {
                LLVMValueRef val = LLVMBuildLoad2(builder, LLVMInt8TypeInContext(context), cell_ptr, "");
                emit_putchar(val);
                break;
            }
            case TOKEN_LISTEN: {
                emit_getchar(cell_ptr);
                break;
            }
            case TOKEN_IFMEOW: {
                // Create loop blocks
                LLVMBasicBlockRef condition = LLVMAppendBasicBlockInContext(context, current_func, "loop_cond");
                LLVMBasicBlockRef body = LLVMAppendBasicBlockInContext(context, current_func, "loop_body");
                LLVMBasicBlockRef end = LLVMAppendBasicBlockInContext(context, current_func, "loop_end");
                
                // Push loop info
                loop_stack[loop_stack_size].condition = condition;
                loop_stack[loop_stack_size].body = body;
                loop_stack[loop_stack_size].end = end;
                loop_stack_size++;
                
                // Jump to condition
                LLVMBuildBr(builder, condition);
                LLVMPositionBuilderAtEnd(builder, condition);
                
                // Check condition
                LLVMValueRef val = LLVMBuildLoad2(builder, LLVMInt8TypeInContext(context), cell_ptr, "");
                LLVMValueRef zero = LLVMConstInt(LLVMInt8TypeInContext(context), 0, 0);
                LLVMValueRef cond = LLVMBuildICmp(builder, LLVMIntEQ, val, zero, "");
                LLVMBuildCondBr(builder, cond, end, body);
                
                // Position at body start
                LLVMPositionBuilderAtEnd(builder, body);
                break;
            }
            case TOKEN_ENDMEOW: {
                if (loop_stack_size == 0) {
                    fprintf(stderr, "Internal error: Unmatched endmeow\n");
                    return;
                }
                
                // Pop loop info
                loop_stack_size--;
                LoopInfo info = loop_stack[loop_stack_size];
                
                // Jump back to condition
                LLVMBuildBr(builder, info.condition);
                
                // Position at loop end
                LLVMPositionBuilderAtEnd(builder, info.end);
                break;
            }
            default:
                break;
        }
    }
    
    // Return from main
    LLVMBuildRetVoid(builder);
}

// ─── COMPILE ──────────────────────────────────────────────────
void codegen_compile(const char* output_name, int optimize) {
    // Write bitcode
    char* bc_file = malloc(strlen(output_name) + 5);
    sprintf(bc_file, "%s.bc", output_name);
    if (LLVMWriteBitcodeToFile(module, bc_file) != 0) {
        fprintf(stderr, "Failed to write bitcode\n");
        free(bc_file);
        return;
    }
    free(bc_file);
    
    // Compile with clang
    char cmd[1024];
    const char* opt_flags = "";
    if (optimize >= 3) opt_flags = "-O3";
    else if (optimize >= 2) opt_flags = "-O2";
    else if (optimize >= 1) opt_flags = "-O1";
    
    snprintf(cmd, sizeof(cmd), 
        "clang %s.bc -o %s %s -lm", 
        output_name, output_name, opt_flags);
    
    int result = system(cmd);
    
    if (result == 0) {
        printf("✅ Compiled to %s\n", output_name);
    } else {
        fprintf(stderr, "⚠️ Compilation failed\n");
    }
    
    // Clean up
    snprintf(cmd, sizeof(cmd), "rm -f %s.bc", output_name);
    system(cmd);
}

void codegen_cleanup() {
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(context);
}