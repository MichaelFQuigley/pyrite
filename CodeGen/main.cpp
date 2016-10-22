/*
 * Entry point for code generation.
 */

#include "ast_walker.h"
#include "codegen_util.h"

int main() {
  llvm::LLVMContext context;
  std::string moduleName = "testRun";
  std::string outputFilename = "../Parser/" + moduleName + ".bc";
  std::string stdlibLocation = "../stdlib/stdlib.ll";
  llvm::Module* module =
      CodeGenUtil::createNewModule(moduleName, stdlibLocation, context);
  AstWalker* ast = new AstWalker(module);
  std::string jsonString;

  if (std::cin) {
    getline(std::cin, jsonString);
    ast->codeGen_top(jsonString);
    CodeGenUtil::dumpIR(module);
    CodeGenUtil::writeToFile(outputFilename, module);
  }
}
