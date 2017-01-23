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
#include "llvm/IR/IRBuilder.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

#include "compile_type.h"

#define GEN_ASSERT(__cond, __msg)    \
  if (!(__cond)) {                   \
    throw std::runtime_error(__msg); \
  }

#define _GEN_ASSERT(__cond) GEN_ASSERT(__cond, "")

#define GEN_FAIL(__msg) GEN_ASSERT(false, (__msg))

/* CodeGenUtil:
 * Keeps track of some compile-time information and provides utility functions
 * for code generation.
 */

namespace codegen {

class CodeGenUtil {
 public:
  CodeGenUtil(llvm::Module* currModule, llvm::LLVMContext* currContext);
  static void writeToFile(const std::string& filename,
                          llvm::Module* currModule);
  static void dumpIR(llvm::Module* currModule);
  llvm::Value* getConstInt64(int64_t val, bool is_signed = true);
  /* getNewHiddenVarName:
   * Returns a new unique name that could be used as a temporary variable.
   */
  std::string getNewHiddenVarName();

  /* createNewModule:
   * Creates new module and links it with the module referenced by
   * stdlibFilename.
   */
  static llvm::Module* createNewModule(
      const std::string& moduleName, const std::string& stdlibFilename,
      const std::string& stdlibTypesFilename, llvm::LLVMContext& context,
      std::map<std::string, CompileType*>* typesMapOut);

  const static std::string LIST_GENERIC_PARAM;
  // Iterator method names for iterable types.
  const static std::string ITERATOR_BEGIN;
  const static std::string ITERATOR_NEXT;
  const static std::string ITERATOR_HASNEXT;
  // Function in c runtime that accesses vtable of an object.
  const static std::string NATIVE_INDEX_INTO_VTABLE;

 private:
  llvm::LLVMContext* currContext;
  llvm::Module* currModule;
  int64_t hidden_var_count;
};
}
#endif
