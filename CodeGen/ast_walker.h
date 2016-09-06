#ifndef AST_WALKER_H
#define AST_WALKER_H

/*
 * ast_walker manages most of the code generation. It takes the JSON output provided by the parser, and generates LLVM code.
 */

#include <json/json.h>

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
#include "llvm/AsmParser/Parser.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include "codegen_util.h"
#include "scope_helper.h"

#define TRY_NODE(JSON_NODE, NODE_NAME)                                \
    do {                                                              \
        if( JSON_NODE [ #NODE_NAME ] != Json::nullValue )             \
        {                                                             \
            return codeGen_ ## NODE_NAME (JSON_NODE [ #NODE_NAME ] ); \
        }                                                             \
    } while(0)

class AstWalker
{
    private:
        llvm::Type* getVoidStarType();
        ScopeHelper* scopeHelper;
        llvm::LLVMContext currContext;
        llvm::IRBuilder<> Builder;
        CodeGenUtil* codeGenHelper;
        std::unique_ptr<llvm::Module> currModule;
        llvm::Value* createObject(llvm::Type* obj_type, bool restore_insert_point);
        CompileVal* createConstObject(std::string type_name, llvm::Value* value);
        CompileVal* createConstObject(CompileType::CommonType commonType, llvm::Value* value);
        bool json_node_has(Json::Value json_node, std::string name, Json::Value* out_node);
        bool load_stdlib(std::string stdlib_filename);
        llvm::BasicBlock* makeBasicBlock(std::string name = "");
        CompileType* makeCompileType(Json::Value json_node);
        //createNativeCall will create a native function call
        llvm::Value* createNativeCall(std::string func_name, 
                                std::vector<llvm::Value*> argsV);
        //createLangCall will create a function call that was 
        //existent in the file being compiled
        CompileVal* createLangCall(CompileVal* func,
                std::vector<CompileVal*>* argsV);
        void pushScope(ScopeNode::ScopeType scopeType, bool funcScopeRetVoid=false);
        void popScope();
        CompileVal* makeFuncProto(Json::Value json_node);
        //startBlock adds block to back of function and starts insert point there.
        void startBlock(llvm::BasicBlock* block);
        //tryGetFunction tries to get the function based on func_name from the current module
        llvm::Function* tryGetFunction(std::string func_name,
            bool raise_fail_exception = true, 
            std::string error_msg="Undefined function");
        std::string createConstructorName(std::string func_name, std::vector<llvm::Value*> argsV);
        //newVarInScope allocates space for a new variable at the top of a function and restores the insert point
        //back to where it was when it was called.
        CompileVal* newVarInScope(std::string varName, CompileVal* value, bool is_definition=true);
        //createBoolCondBr takes a value of type struct.Bool (the struct type in the stdlib),
        //extracts the raw boolean value, and creates a conditional branch based on that.
        void createBoolCondBr(llvm::Value* Bool, 
                llvm::BasicBlock* trueBlock,
                llvm::BasicBlock* falseBlock);
        llvm::Value* createGlobalFunctionConst(std::string funcName, CompileVal* func);
        void addFuncPtr(std::string funcName, CompileVal* func);
        void handleAssignLhs(Json::Value assignLhs, CompileVal* rhs);
        CompileVal* createReturn(CompileVal* val);
    public:
        void writeToFile(std::string filename);
        AstWalker(std::string filename, std::string stdlib_filename);
        void codeGen_top(std::string json_string);
        CompileVal* codeGen_initial(Json::Value json_node);
        /*
         * codeGen_StmtsOp:
         * returns last llvm::Value from the array of simple statements.
         */
        CompileVal* codeGen_StmtsOp(Json::Value json_node);
        CompileVal* codeGen_ExprOp(Json::Value json_node);
        CompileVal* codeGen_VarDef(Json::Value json_node);
        CompileVal* codeGen_BinOp(Json::Value json_node);
        CompileVal* codeGen_AtomOp(Json::Value json_node);
        CompileVal* codeGen_ForOp(Json::Value json_node);
        CompileVal* codeGen_WhileOp(Json::Value json_node);
        CompileVal* codeGen_IfOp(Json::Value json_node);
        CompileVal* codeGen_FuncDef(Json::Value json_node);
        CompileVal* codeGen_TypedArg(Json::Value json_node);
        CompileVal* codeGen_ListOp(Json::Value json_node);
        CompileVal* codeGen_BracExpr(Json::Value json_node);

        Json::Value generateFromJson(std::string json_string);
        void dumpIR();
        llvm::Module* getModule();
        llvm::LLVMContext* getContext();
};

#endif
