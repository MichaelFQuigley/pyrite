#include "codegen_util.h"

CodeGenUtil::CodeGenUtil(llvm::Module* currModule,
                         llvm::LLVMContext* currContext) {
  this->currModule = currModule;
  this->currContext = currContext;
  hidden_var_count = 0;
}

std::string CodeGenUtil::getNewHiddenVarName() {
  return "$temp_" + std::to_string(hidden_var_count++);
}

void CodeGenUtil::writeToFile(std::string filename, llvm::Module* currModule) {
  std::error_code errs;
  // TODO make raw_fd_ostream decl more portable
  llvm::raw_fd_ostream file(llvm::StringRef(filename), errs,
                            (llvm::sys::fs::OpenFlags)8);
  GEN_ASSERT(errs.value() == 0, "Error writing file.");
  llvm::WriteBitcodeToFile(currModule, file);
}

void CodeGenUtil::dumpIR(llvm::Module* currModule) { currModule->dump(); }

llvm::Value* CodeGenUtil::getConstInt64(int64_t val, bool is_signed) {
  return llvm::ConstantInt::get(*currContext, llvm::APInt(64, val, is_signed));
}

llvm::Value* CodeGenUtil::generateString(std::string str) {
  llvm::ArrayType* ArrayTy = llvm::ArrayType::get(
      llvm::IntegerType::get(*currContext, 8), str.length() + 1);
  llvm::GlobalVariable* str_gvar = new llvm::GlobalVariable(
      *currModule, ArrayTy, true, llvm::GlobalValue::PrivateLinkage, nullptr,
      ".str");

  llvm::Constant* str_arr =
      llvm::ConstantDataArray::getString(*currContext, str);
  llvm::ConstantInt* const_int64 =
      llvm::ConstantInt::get(*currContext, llvm::APInt(64, 0, true));
  std::vector<llvm::Constant*> indices;
  indices.push_back(const_int64);
  indices.push_back(const_int64);
  llvm::Constant* result =
      llvm::ConstantExpr::getGetElementPtr(str_gvar, indices);
  str_gvar->setInitializer(str_arr);

  return result;
}

llvm::Module* CodeGenUtil::createNewModule(std::string moduleName,
                                           std::string stdlibFilename,
                                           llvm::LLVMContext& context) {
  llvm::SMDiagnostic Err;
  llvm::Module* resultModule = new llvm::Module(moduleName, context);
  std::unique_ptr<llvm::Module> stdlibMod =
      llvm::parseIRFile(stdlibFilename, Err, context);

  if (stdlibMod == nullptr) {
    GEN_FAIL("Standard library does not exist at location " + stdlibFilename);
  }

  llvm::Linker::LinkModules(resultModule, stdlibMod.get());

  return resultModule;
}
