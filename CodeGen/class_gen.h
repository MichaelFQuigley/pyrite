#ifndef CLASS_GEN_H
#define CLASS_GEN_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

namespace codegen {

class ClassGen {
 public:
  ClassGen(llvm::IRBuilder<>& builder, llvm::LLVMContext& context);
  void generateEmptyClass(const std::string& className);

 private:
  llvm::IRBuilder<>& builder;
  llvm::LLVMContext& context;
};

}  // namespace codegen
#endif
