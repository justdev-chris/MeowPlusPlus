#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Analysis.h>
#include "meowplus.h"

// ─── GLOBALS ──────────────────────────────────────────────────
LLVMModuleRef module;
static LLVMBuilderRef builder;
static LLVMValueRef tape_ptr;
static LLVMValueRef ptr_var;
static LLVMValueRef current_func;
static LLVMBasicBlockRef current_bb;
static LLVMContextRef context;

// FIX: hoisted type refs so we don't recreate/intern them on every call
static LLVMTypeRef tape_type;
static LLVMTypeRef i8_type;
static LLVMTypeRef i32_type;
static LLVMTypeRef putchar_type;
static LLVMTypeRef getchar_type;

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

    // FIX: intern the primitive types once
    i8_type  = LLVMInt8TypeInContext(context);
    i32_type = LLVMInt32TypeInContext(context);

    // FIX: tape_type built once and reused everywhere
    tape_type = LLVMArrayType(i8_type, TAPE_SIZE);
    tape_ptr = LLVMAddGlobal(module, tape_type, "tape");
    LLVMSetLinkage(tape_ptr, LLVMPrivateLinkage);
    LLVMSetInitializer(tape_ptr, LLVMConstNull(tape_type));
    
    // Create pointer variable (i32)
    ptr_var = LLVMAddGlobal(module, i32_type, "ptr");
    LLVMSetLinkage(ptr_var, LLVMPrivateLinkage);
    LLVMSetInitializer(ptr_var, LLVMConstInt(i32_type, 0, 0));
    
    // FIX: build and store the function types for putchar/getchar
    LLVMTypeRef putchar_args[] = { i32_type };
    putchar_type = LLVMFunctionType(i32_type, putchar_args, 1, 0);
    LLVMAddFunction(module, "putchar", putchar_type);
    
    getchar_type = LLVMFunctionType(i32_type, NULL, 0, 0);
    LLVMAddFunction(module, "getchar", getchar_type);
}

// ─── EMIT PUTCHAR ─────────────────────────────────────────────
static void emit_putchar(LLVMValueRef val) {
    LLVMValueRef putchar = LLVMGetNamedFunction(module, "putchar");
    LLVMValueRef int_arg = LLVMBuildZExt(builder, val, i32_type, "");
    // FIX: pass the function type (putchar_type), not i32_type
    LLVMBuildCall2(builder, putchar_type, putchar, &int_arg, 1, "");
}

// ─── EMIT GETCHAR ─────────────────────────────────────────────
static void emit_getchar(LLVMValueRef cell_ptr) {
    LLVMValueRef getchar = LLVMGetNamedFunction(module, "getchar");
    // FIX: pass getchar_type, not i32_type
    LLVMValueRef result = LLVMBuildCall2(builder, getchar_type, getchar, NULL, 0, "");
    // FIX: mask low byte before truncating so EOF doesn't become 0xFF
    LLVMValueRef masked = LLVMBuildAnd(builder, result,
        LLVMConstInt(i32_type, 0xFF, 0), "");
    result = LLVMBuildTrunc(builder, masked, i8_type, "");
    LLVMBuildStore(builder, result, cell_ptr);
}

// ─── GET CELL POINTER ─────────────────────────────────────────
// FIX: GEP directly into the global tape array.
// Do NOT load the array by value — that produces a temporary with no
// stable address and LLVM will choke on the resulting GEP.
static LLVMValueRef get_cell_ptr(LLVMValueRef ptr_val) {
    LLVMValueRef indices[] = {
        LLVMConstInt(i32_type, 0, 0),
        ptr_val
    };
    return LLVMBuildGEP2(builder, tape_type, tape_ptr, indices, 2, "cell_ptr");
}

// ─── GENERATE MAIN FUNCTION ──────────────────────────────────
LLVMValueRef codegen_create_main() {
    LLVMTypeRef main_type = LLVMFunctionType(i32_type, NULL, 0, 0);
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
    LLVMValueRef ptr = LLVMConstInt(i32_type, 0, 0);
    LLVMBuildStore(builder, ptr, ptr_var);
    
    // Get initial cell pointer
    LLVMValueRef cell_ptr = get_cell_ptr(ptr);
    
    // Process tokens
    for (int i = 0; i < count; i++) {
        switch (tokens[i].type) {
            case TOKEN_PURR: {
                LLVMValueRef val = LLVMBuildLoad2(builder, i8_type, cell_ptr, "");
                val = LLVMBuildAdd(builder, val, LLVMConstInt(i8_type, 1, 0), "");
                LLVMBuildStore(builder, val, cell_ptr);
                break;
            }
            case TOKEN_HISS: {
                LLVMValueRef val = LLVMBuildLoad2(builder, i8_type, cell_ptr, "");
                val = LLVMBuildSub(builder, val, LLVMConstInt(i8_type, 1, 0), "");
                LLVMBuildStore(builder, val, cell_ptr);
                break;
            }
            case TOKEN_PAW: {
                ptr = LLVMBuildAdd(builder, ptr, LLVMConstInt(i32_type, 1, 0), "");
                LLVMBuildStore(builder, ptr, ptr_var);
                cell_ptr = get_cell_ptr(ptr);
                break;
            }
            case TOKEN_PAWBACK: {
                ptr = LLVMBuildSub(builder, ptr, LLVMConstInt(i32_type, 1, 0), "");
                LLVMBuildStore(builder, ptr, ptr_var);
                cell_ptr = get_cell_ptr(ptr);
                break;
            }
            case TOKEN_MEOW: {
                LLVMValueRef val = LLVMBuildLoad2(builder, i8_type, cell_ptr, "");
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
                LLVMValueRef val = LLVMBuildLoad2(builder, i8_type, cell_ptr, "");
                LLVMValueRef cond = LLVMBuildICmp(builder, LLVMIntEQ, val, LLVMConstInt(i8_type, 0, 0), "");
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
    
    // Return 0 from main
    LLVMBuildRet(builder, LLVMConstInt(i32_type, 0, 0));
}

// ─── COMPILE ──────────────────────────────────────────────────
void codegen_compile(const char* output_name, int optimize) {
    // FIX: verify the IR before we do anything with it. This catches
    // malformed instructions (bad call types, GEPs into temporaries,
    // mismatched types, etc.) with a real error message instead of a
    // segfault later in clang.
    char* verify_err = NULL;
    if (LLVMVerifyModule(module, LLVMReturnStatusAction, &verify_err)) {
        fprintf(stderr, "⚠️ LLVM IR verification failed:\n%s\n", verify_err);
        LLVMDisposeMessage(verify_err);
        return;
    }
    LLVMDisposeMessage(verify_err);

    // Write bitcode
    char* bc_file = malloc(strlen(output_name) + 5);
    sprintf(bc_file, "%s.bc", output_name);
    if (LLVMWriteBitcodeToFile(module, bc_file) != 0) {
        fprintf(stderr, "Failed to write bitcode\n");
        free(bc_file);
        return;
    }
    free(bc_file);
    
    // Compile with clang (not gcc - gcc can't handle .bc files)
    char cmd[1024];
    const char* opt_flags = "";
    if (optimize >= 3) opt_flags = "-O3";
    else if (optimize >= 2) opt_flags = "-O2";
    else if (optimize >= 1) opt_flags = "-O1";
    
    // Use clang to compile bitcode to executable
    snprintf(cmd, sizeof(cmd), 
        "clang %s.bc -o %s %s -lm", 
        output_name, output_name, opt_flags);
    
    int result = system(cmd);
    
    if (result == 0) {
        printf("✅ Compiled to %s\n", output_name);
    } else {
        fprintf(stderr, "⚠️ Compilation failed with code %d\n", result);
        fprintf(stderr, "Command: %s\n", cmd);
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