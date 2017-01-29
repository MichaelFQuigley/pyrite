#include "type_reader.h"
#include "codegen_util.h"

namespace codegen {

const std::string CodeGenUtil::LIST_GENERIC_PARAM = "T";
const std::string CodeGenUtil::ITERATOR_BEGIN = "begin";
const std::string CodeGenUtil::ITERATOR_NEXT = "next";
const std::string CodeGenUtil::ITERATOR_HASNEXT = "hasNext";
const std::string CodeGenUtil::NATIVE_INDEX_INTO_VTABLE = "indexIntoVtable";
const int64_t CodeGenUtil::END_OF_BASE_VALS = 3;

CodeGenUtil::CodeGenUtil(llvm::Module *currModule,
                         llvm::LLVMContext *currContext,
                         llvm::IRBuilder<> &Builder,
                         std::map<std::string, CompileType *> moduleTypes)
    : Builder(Builder), moduleTypes(moduleTypes) {
  this->currModule = currModule;
  this->currContext = currContext;
  hidden_var_count = 0;
}

std::string CodeGenUtil::getNewHiddenVarName() {
  return "$temp_" + std::to_string(hidden_var_count++);
}

void CodeGenUtil::writeToFile(const std::string &filename,
                              llvm::Module *currModule) {
  std::error_code errs;
  // TODO make raw_fd_ostream decl more portable
  llvm::raw_fd_ostream file(llvm::StringRef(filename), errs,
                            (llvm::sys::fs::OpenFlags)8);
  GEN_ASSERT(errs.value() == 0, "Error writing file.");
  llvm::WriteBitcodeToFile(currModule, file);
}

void CodeGenUtil::dumpIR(llvm::Module *currModule) { currModule->dump(); }

llvm::Value *CodeGenUtil::getConstInt64(int64_t val, bool is_signed) {
  return llvm::ConstantInt::get(*currContext, llvm::APInt(64, val, is_signed));
}

llvm::Module *CodeGenUtil::createNewModule(
    const std::string &moduleName, const std::string &stdlibFilename,
    const std::string &stdlibTypesFilename, llvm::LLVMContext &context,
    std::map<std::string, CompileType *> *typesMapOut) {
  llvm::SMDiagnostic Err;
  llvm::Module *resultModule = new llvm::Module(moduleName, context);
  std::unique_ptr<llvm::Module> stdlibMod =
      llvm::parseIRFile(stdlibFilename, Err, context);

  if (stdlibMod == nullptr) {
    GEN_FAIL("Error loading standard library at location " + stdlibFilename);
  }
  llvm::Linker::linkModules(*resultModule, std::move(stdlibMod));

  TypeReader typeReader;
  std::ifstream inFile;
  inFile.open(stdlibTypesFilename);
  GEN_ASSERT(inFile.is_open(),
             "Could not open Pyrite type info file: " + stdlibTypesFilename);
  typeReader.compileTypesFromFile(inFile);
  auto typesMap = typeReader.getCompileTypesMap();
  typesMapOut->insert(typesMap.begin(), typesMap.end());

  return resultModule;
}

CompileVal *CodeGenUtil::createVtableAccess(CompileVal *obj,
                                            const std::string &functionName) {
  if (!obj->getIsBoxed()) {
    obj = boxValue(obj);
  }
  int methodVtableIndex =
      obj->getCompileType()->getMethodIndex(functionName, nullptr);
  GEN_ASSERT(methodVtableIndex != -1,
             "Method " + obj->getCompileType()->getTypeName() + "." +
                 functionName + " is not found!");
  CompileType const *methodType =
      obj->getCompileType()->getMethodType(functionName);
  llvm::Value *irMethodIndex = getConstInt64(methodVtableIndex);
  llvm::Value *methodHandle =
      createNativeCall(CodeGenUtil::NATIVE_INDEX_INTO_VTABLE,
                       {obj->getRawValue(), irMethodIndex});
  // TODO typecheck args vs method contract.
  llvm::Value *castedValue = Builder.CreatePointerCast(
      methodHandle, llvm::PointerType::getUnqual(CompileType::asRawFunctionType(
                        methodType, Builder.getInt8PtrTy(), *currContext)));
  // XXX handle null return type.
  return new CompileVal(castedValue, methodType, /*isBoxed=*/false);
}

llvm::Function *CodeGenUtil::tryGetFunction(std::string func_name,
                                            bool raise_fail_exception,
                                            std::string error_msg) {
  std::string full_func_name = func_name;

  auto func = currModule->getFunction(full_func_name);

  if (raise_fail_exception) {
    GEN_ASSERT(func != nullptr, full_func_name + " " + error_msg);
  }

  return func;
}

llvm::Value *CodeGenUtil::createNativeCall(
    std::string func_name, const std::vector<llvm::Value *> &argsV) {
  auto func = tryGetFunction(func_name);
  llvm::Value *ret_val = Builder.CreateCall(func, argsV);

  return ret_val;
}

CompileVal *CodeGenUtil::createLiteral(CompileType::CommonType commonType,
                                       llvm::Value *raw_value) {
  return createLiteral(CompileType::getCommonTypeName(commonType), raw_value);
}

CompileVal *CodeGenUtil::createLiteral(const std::string &typeName,
                                       llvm::Value *rawValue) {
  llvm::Value *objectVal = createInitCall(typeName, rawValue);
  return new CompileVal(objectVal, getCompileType(typeName));
}

llvm::Value *CodeGenUtil::createInitCall(const std::string &typeName,
                                         llvm::Value *rawValue) {
  const std::string init_func_name = "init_" + typeName;

  llvm::BasicBlock *originalBlock = Builder.GetInsertBlock();
  llvm::Value *result = createNativeCall(init_func_name, {rawValue});

  Builder.SetInsertPoint(originalBlock);
  return result;
}

CompileVal *CodeGenUtil::boxValue(CompileVal *compileVal) {
  GEN_ASSERT(!compileVal->getIsBoxed(), "Tried to box an already boxed value.")
  CompileType *compileType = compileVal->getCompileType();
  if (compileType->isFunctionType()) {
    llvm::Value *castedValue = Builder.CreatePointerCast(
        compileVal->getRawValue(), Builder.getInt8PtrTy());
    return new CompileVal(
        createInitCall(compileVal->getCompileType()->getTypeName(),
                       castedValue),
        compileVal->getCompileType(), /*isBoxed=*/true);
  } else {
    GEN_FAIL("Only functions can be boxed.");
  }
}

CompileVal *CodeGenUtil::unboxValue(CompileVal *compileVal) {
  GEN_ASSERT(compileVal->getIsBoxed(),
             "Tried to unbox an already unboxed value.")
  CompileType *compileType = compileVal->getCompileType();

  if (compileType->isFunctionType()) {
    // Casts raw value to void**
    llvm::Value *rawCompileValPtr = Builder.CreatePointerCast(
        compileVal->getRawValue(),
        llvm::PointerType::getUnqual(Builder.getInt8PtrTy()));
    // Gets element pointer and loads value at end of base vals offset.
    llvm::Value *rawUncastFuncVal =
        Builder.CreateLoad(Builder.CreateInBoundsGEP(
            rawCompileValPtr,
            {getConstInt64(CodeGenUtil::END_OF_BASE_VALS, false)}));
    return new CompileVal(rawUncastFuncVal, compileType, /*isBoxed=*/false);
  } else {
    GEN_FAIL("Only functions can be unboxed.");
  }
}

CompileType *CodeGenUtil::getCompileType(const std::string &typeName) {
  GEN_ASSERT(typeName != CompileType::getCommonTypeName(
                             CompileType::CommonType::FUNCTION),
             "Function types should not be accessed this way.");
  GEN_ASSERT(moduleTypes.find(typeName) != moduleTypes.end(),
             "Type " + typeName + " not found.");
  return moduleTypes[typeName];
}

CompileType *CodeGenUtil::getCompileType(CompileType::CommonType typeName) {
  return getCompileType(CompileType::getCommonTypeName(typeName));
}
}
