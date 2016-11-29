/*
 * Entry point for code generation.
 */

#include "ast_walker.h"
#include "compile_type.h"
#include "codegen_util.h"

int main() {
  llvm::LLVMContext context;
  std::string moduleName = "testRun";
  std::string outputFilename = "../Parser/" + moduleName + ".bc";
  std::string stdlibLocation = "../stdlib/stdlib.ll";
  std::string stdlibTypesLocation = "../stdlib/lib_types.pyti";
  std::map<std::string, codegen::CompileType *> typesMap;
  llvm::Module *module = codegen::CodeGenUtil::createNewModule(
      moduleName, stdlibLocation, stdlibTypesLocation, context, &typesMap);
  codegen::AstWalker *ast = new codegen::AstWalker(module, &typesMap);
  std::string jsonString;

  if (std::cin) {
    getline(std::cin, jsonString);
    ast->codeGen_top(jsonString);
    codegen::CodeGenUtil::dumpIR(module);
    codegen::CodeGenUtil::writeToFile(outputFilename, module);
  }
}
