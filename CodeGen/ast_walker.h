#ifndef AST_WALKER_H
#define AST_WALKER_H

/*
 * ast_walker manages most of the code generation.
 * It takes the JSON output provided by the parser, and generates LLVM code.
 */

#include "jsoncpp/json/json.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Value.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include "codegen_util.h"
#include "scope_helper.h"

#define TRY_NODE(JSON_NODE, NODE_NAME)                   \
  do {                                                   \
    if (JSON_NODE[#NODE_NAME] != Json::nullValue) {      \
      return codeGen_##NODE_NAME(JSON_NODE[#NODE_NAME]); \
    }                                                    \
  } while (0)

namespace codegen {

class AstWalker {
 private:
  ScopeHelper* scopeHelper;
  llvm::LLVMContext& currContext;
  llvm::IRBuilder<> Builder;
  CodeGenUtil* codeGenHelper;
  llvm::Module* currModule;
  std::map<std::string, CompileType*> moduleTypes;
  llvm::Value* createObject(llvm::Type* obj_type, bool restore_insert_point);
  CompileVal* createLiteral(const std::string& type_name, llvm::Value* value);
  CompileVal* createLiteral(CompileType::CommonType commonType,
                            llvm::Value* value);
  llvm::Value* createGlobalFunctionConst(const std::string& funcName,
                                         CompileVal* func);
  bool jsonNode_has(Json::Value& jsonNode, const std::string& name,
                    Json::Value* out_node);
  // jsonNodeMustHave fails if there is not a node for the provided name.
  Json::Value jsonNodeGet(Json::Value& node, const std::string& name);
  bool load_stdlib(std::string stdlib_filename);
  llvm::BasicBlock* makeBasicBlock(std::string name = "");
  CompileType* makeCompileType(Json::Value& jsonNode);
  // createNativeCall will create a native function call
  llvm::Value* createNativeCall(std::string func_name,
                                const std::vector<llvm::Value*>& argsV);
  // createLangCall will create a function call that was
  // existent in the file being compiled
  CompileVal* createLangCall(CompileVal* func,
                             const std::vector<CompileVal*>& argsV);
  // Creates method call on class represented by thisObj. The this pointer is inserted into the args list automatically.
  CompileVal* createClassMethodCall(
      const std::string& methodName, CompileVal* thisObj, const std::vector<CompileVal*>& args={});
  void pushScope(ScopeNode::ScopeType scopeType, bool funcScopeRetVoid = false);
  void popScope();
  CompileVal* makeFuncProto(Json::Value& jsonNode);
  // startBlock adds block to back of function and starts insert point there.
  void startBlock(llvm::BasicBlock* block);
  // tryGetFunction tries to get the function based on func_name from the
  // current module
  llvm::Function* tryGetFunction(std::string func_name,
                                 bool raise_fail_exception = true,
                                 std::string error_msg = "Undefined function");
  std::string createConstructorName(std::string func_name,
                                    std::vector<llvm::Value*> argsV);
  // newVarInScope allocates space for a new variable at the top of a function
  // and restores the insert point
  // back to where it was when it was called.
  CompileVal* newVarInScope(const std::string& varName, CompileVal* value,
                            bool is_definition = true);
  // createBoolCondBr takes a value of type struct.Bool (the struct type in the
  // stdlib),
  // extracts the raw boolean value, and creates a conditional branch based on
  // that.
  void createBoolCondBr(CompileVal* Bool, llvm::BasicBlock* trueBlock,
                        llvm::BasicBlock* falseBlock);
  void addFuncPtr(std::string funcName, CompileVal* func);
  void handleAssignLhs(Json::Value& assignLhs, CompileVal* rhs);
  CompileVal* createReturn(CompileVal* val);
  /*
   * createVtableAccess:
   * Creates function access from provided object.
   */
  CompileVal* createVtableAccess(CompileVal* obj,
                                 const std::string& functionName);
  CompileType* getCompileType(const std::string& typeName);
  /*
   * handleTrailers:
   * Takes json list of possible trailer values and returns a result value
   * derived
   * from the currVal with the additional trailers.
   */
  CompileVal* handleTrailers(Json::Value& trailers, CompileVal* currVal);

 public:
  void writeToFile(std::string filename);
  AstWalker(llvm::Module* output_module,
            std::map<std::string, CompileType*> const* moduleTypes);
  void codeGen_top(std::string jsonString);
  CompileVal* codeGen_initial(Json::Value& jsonNode);
  /*
   * codeGen_StmtsOp:
   * returns last llvm::Value from the array of simple statements.
   */
  CompileVal* codeGen_StmtsOp(Json::Value& jsonNode);
  CompileVal* codeGen_ExprOp(Json::Value& jsonNode);
  CompileVal* codeGen_VarDef(Json::Value& jsonNode);
  CompileVal* codeGen_BinOp(Json::Value& jsonNode);
  CompileVal* codeGen_AtomOp(Json::Value& jsonNode);
  CompileVal* codeGen_ForOp(Json::Value& jsonNode);
  CompileVal* codeGen_WhileOp(Json::Value& jsonNode);
  CompileVal* codeGen_IfOp(Json::Value& jsonNode);
  CompileVal* codeGen_FuncDef(Json::Value& jsonNode);
  CompileVal* codeGen_TypedArg(Json::Value& jsonNode);
  CompileVal* codeGen_ListOp(Json::Value& jsonNode);
  CompileVal* codeGen_BracExpr(Json::Value& jsonNode);
  CompileVal* codeGen_UnOp(Json::Value& jsonNode);
  CompileVal* codeGen_ListGen(Json::Value& jsonNode);
  //  CompileVal* codeGen_ClassDef(Json::Value& jsonNode);

  Json::Value generateFromJson(std::string jsonString);
  void dumpIR();
  llvm::Module* getModule();
  llvm::LLVMContext* getContext();
};
}
#endif
