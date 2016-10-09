#ifndef CODEGEN_UTIL_H
#define CODEGEN_UTIL_H
/*
 * Utility functions for code generation.
 */

#include <iostream>
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Type.h"



#define GEN_ASSERT(__cond, __msg)           \
    if(!(__cond))                           \
    {                                       \
        std::cout << (__msg) << std::endl;  \
        throw;                              \
    }

#define _GEN_ASSERT(__cond) \
    GEN_ASSERT(__cond, "")

#define GEN_FAIL(__msg) \
    GEN_ASSERT(false, (__msg))

/* CodeGenUtil:
 * Keeps track of some compile-time information and provides utility functions
 * for code generation.
 */
class CodeGenUtil
{
    public:
        CodeGenUtil(llvm::Module* currModule, llvm::LLVMContext* currContext);
        static void writeToFile(std::string filename, llvm::Module* currModule);
        static void dumpIR(llvm::Module* currModule);
        llvm::Value* getConstInt64(int64_t val, bool is_signed=true);
        bool load_stdlib(std::string stdlib_filename);
        llvm::Value* generateString(std::string str);
        /* getNewHiddenVarName:
         * Returns a new unique name that could be used as a temporary variable.
         */
        std::string getNewHiddenVarName();
    private:
        llvm::LLVMContext* currContext;
        llvm::Module* currModule;
        int64_t hidden_var_count;
};

#endif
