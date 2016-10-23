/*
 * Code generation for classes.
 */

#include "class_gen.h"

namespace codegen {

ClassGen::ClassGen(llvm::IRBuilder<>& builder, llvm::LLVMContext& context)
    : builder(builder), context(context) {}

void ClassGen::generateEmptyClass(const std::string& className) {
  llvm::StructType* classStruct = llvm::StructType::create(context, className);
  llvm::Type* uninitFuncPtr = llvm::PointerType::getUnqual(
      llvm::FunctionType::get(llvm::Type::getVoidTy(context), false));
  llvm::Type* getRefsFuncPtr =
      llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(
          llvm::FunctionType::get(llvm::Type::getVoidTy(context), false)));
  classStruct->setBody({uninitFuncPtr, getRefsFuncPtr});
}

}  // namespace codegen
