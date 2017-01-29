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
                         llvm::LLVMContext *currContext) {
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
}
