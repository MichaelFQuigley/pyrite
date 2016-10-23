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
  llvm::Module *module = codegen::CodeGenUtil::createNewModule(
      moduleName, stdlibLocation, context);
  codegen::AstWalker *ast = new codegen::AstWalker(module);
  std::string jsonString;

  if (std::cin) {
    getline(std::cin, jsonString);
    ast->codeGen_top(jsonString);
    //    CodeGenUtil::dumpIR(module);
    codegen::CodeGenUtil::writeToFile(outputFilename, module);
  }
}
