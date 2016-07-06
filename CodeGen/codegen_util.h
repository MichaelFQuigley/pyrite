#ifndef CODEGEN_UTIL_H
#define CODEGEN_UTIL_H

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

class CodeGenUtil
{
    public:
        static void writeToFile(std::string filename, llvm::Module* currModule);
        static void dumpIR(llvm::Module* currModule);
        static llvm::Value* getConstInt64(llvm::LLVMContext* currContext, int64_t val, bool is_signed=true);
        static bool load_stdlib(std::string stdlib_filename, 
                llvm::Module* currModule, 
                llvm::LLVMContext* currContext);
        static std::string getTypeStr(llvm::Value* val, 
                bool with_struct_prefix=true);
        static llvm::Value* generateString(llvm::Module* module, std::string str);
        static std::string typeStrFromStr(std::string type_name);
        static void assertType(std::string type_name, 
                llvm::Value* val, 
                std::string error_msg="Types are not matching.");
        static void assertType(llvm::Value* valA, 
                llvm::Value* valB, 
                std::string error_msg="Types are not matching.");
        static uint64_t getPointedToStructSize(llvm::Module * module, llvm::Value* val);
};

#endif
