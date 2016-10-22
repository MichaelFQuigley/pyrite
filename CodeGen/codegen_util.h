#ifndef CODEGEN_UTIL_H
#define CODEGEN_UTIL_H

/*
 * Utility functions for code generation.
 */

#include <iostream>
#include "llvm/ADT/STLExtras.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

#define GEN_ASSERT(__cond, __msg)      \
  if (!(__cond)) {                     \
    std::cout << (__msg) << std::endl; \
    throw;                             \
  }

#define _GEN_ASSERT(__cond) GEN_ASSERT(__cond, "")

#define GEN_FAIL(__msg) GEN_ASSERT(false, (__msg))

/* CodeGenUtil:
 * Keeps track of some compile-time information and provides utility functions
 * for code generation.
 */
class CodeGenUtil {
 public:
  CodeGenUtil(llvm::Module* currModule, llvm::LLVMContext* currContext);
  static void writeToFile(std::string filename, llvm::Module* currModule);
  static void dumpIR(llvm::Module* currModule);
  llvm::Value* getConstInt64(int64_t val, bool is_signed = true);
  llvm::Value* generateString(std::string str);
  /* getNewHiddenVarName:
   * Returns a new unique name that could be used as a temporary variable.
   */
  std::string getNewHiddenVarName();

  /* createNewModule:
   * Creates new module and links it with the module referenced by
   * stdlibFilename.
   */
  static llvm::Module* createNewModule(std::string moduleName,
                                       std::string stdlibFilename,
                                       llvm::LLVMContext& context);

 private:
  llvm::LLVMContext* currContext;
  llvm::Module* currModule;
  int64_t hidden_var_count;
};

#endif
