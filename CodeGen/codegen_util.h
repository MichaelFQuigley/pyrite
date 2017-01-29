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
  CodeGenUtil(llvm::Module* currModule, llvm::LLVMContext* currContext,
              llvm::IRBuilder<>& Builder,
              std::map<std::string, CompileType*> moduleTypes);
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

  /*
   * createVtableAccess:
   * Creates function access from provided object.
   */
  CompileVal* createVtableAccess(CompileVal* obj,
                                 const std::string& functionName);

  /* tryGetFunction:
   * Tries to get the function based on func_name from the
   * current module.
   */
  llvm::Function* tryGetFunction(std::string func_name,
                                 bool raise_fail_exception = true,
                                 std::string error_msg = "Undefined function");

  /* createNativeCall:
   * Will create a native function call.
   */
  llvm::Value* createNativeCall(std::string func_name,
                                const std::vector<llvm::Value*>& argsV);

  /*
   * boxValue:
   * Boxes rawValue of the provided compileVal. For example, a raw function
   * pointer would be boxed inside of a Function object.
   * The resulting object is a new CompileVal object.
   */
  CompileVal* boxValue(CompileVal* compileVal);

  /*
   * unboxValue:
   * Unboxes rawValue of the provided compileVal.
   */
  CompileVal* unboxValue(CompileVal* compileVal);

  /*
   * boxIfNot:
   * Returns boxed value always.
   */
  CompileVal* boxIfNot(CompileVal* compileVal);

  /*
   * unboxIfNot:
   * Returns unboxed value always.
   */
  CompileVal* unboxIfNot(CompileVal* compileVal);

  /*
   * createInitCall:
   * Creates call to low level initialization routine for type indicated by
   * typeName.
   * rawValue is the single raw parameter to the init routine.
   * Returns the result of the init routine.
   */
  llvm::Value* createInitCall(const std::string& typeName,
                              llvm::Value* rawValue);

  CompileVal* createLiteral(const std::string& type_name, llvm::Value* value);
  CompileVal* createLiteral(CompileType::CommonType commonType,
                            llvm::Value* value);

  CompileType* getCompileType(const std::string& typeName);
  CompileType* getCompileType(CompileType::CommonType typeName);

  CompileVal* generateUnboxedIntBinOp(CompileVal* lhs, CompileVal* rhs,
                                      const std::string& op);
  // Name of the single generic parameter in a List.
  const static std::string LIST_GENERIC_PARAM;
  // Iterator method names for iterable types.
  const static std::string ITERATOR_BEGIN;
  const static std::string ITERATOR_NEXT;
  const static std::string ITERATOR_HASNEXT;

  // Binary operator function names.
  const static std::string ADD_FUNC;
  const static std::string SUB_FUNC;
  const static std::string MUL_FUNC;
  const static std::string DIV_FUNC;
  const static std::string MOD_FUNC;
  const static std::string AND_FUNC;
  const static std::string OR_FUNC;
  const static std::string XOR_FUNC;
  const static std::string CMPLT_FUNC;
  const static std::string CMPLE_FUNC;
  const static std::string CMPEQ_FUNC;
  const static std::string CMPGT_FUNC;
  const static std::string CMPGE_FUNC;
  const static std::string CMPNE_FUNC;

  // Function in c runtime that accesses vtable of an object.
  const static std::string NATIVE_INDEX_INTO_VTABLE;
  // End of base values offset (divided by 8) in Base struct.
  const static int64_t END_OF_BASE_VALS;

 private:
  llvm::LLVMContext* currContext;
  llvm::Module* currModule;
  llvm::IRBuilder<>& Builder;
  std::map<std::string, CompileType*> moduleTypes;
  int64_t hidden_var_count;
};
}
#endif
