#ifndef AST_WALKER_H
#define AST_WALKER_H

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
        ScopeHelper* scopeHelper;
        llvm::LLVMContext currContext;
        llvm::IRBuilder<> Builder;
        std::unique_ptr<llvm::Module> currModule;
        llvm::Value* createObject(llvm::Type* obj_type, bool restore_insert_point);
        llvm::Value* createConstObject(std::string type_name, llvm::Value* value);
        bool json_node_has(Json::Value json_node, std::string name, Json::Value* out_node);
        bool load_stdlib(std::string stdlib_filename);
        llvm::BasicBlock* makeBasicBlock(std::string name = "");
        //createCall will create a function call with the convention that
        //the function returns void, and the last parameter is the out parameter
        //where a return value would be stored. This function will allocate space
        //for the out parameter and return a pointer to this space. Note that the 
        //argsV parameter should only include the input params to the function
        //being called. If no return value is needed, then this function returns nullptr.
        llvm::Value* createCall(std::string func_name, 
                                std::vector<llvm::Value*> argsV, 
                                bool restore_insert_point=true);
        llvm::StructType* getTypeFromStr(std::string typeName);
        llvm::Type* getPtrTypeFromStr(std::string typeName);
        llvm::Function* makeFuncProto(Json::Value json_node, std::string result_param_name);
        //startBlock adds block to back of function and starts insert point there.
        void startBlock(llvm::BasicBlock* block);
        //tryGetFunction tries to get the function based on func_name from the current module
        llvm::Function* tryGetFunction(std::string func_name,
            bool raise_fail_exception = true, 
            std::string error_msg="Undefined function");
        std::string createConstructorName(std::string func_name, std::vector<llvm::Value*> argsV);
        llvm::Value* newVarInScope(std::string varName, llvm::Value* value);
        //createBoolCondBr takes a value of type struct.Bool (the struct type in the stdlib),
        //extracts the raw boolean value, and creates a conditional branch based on that.
        void createBoolCondBr(llvm::Value* Bool, 
                llvm::BasicBlock* trueBlock,
                llvm::BasicBlock* falseBlock);
    public:
        void writeToFile(std::string filename);
        AstWalker(std::string filename, std::string stdlib_filename);
        void codeGen_top(std::string json_string);
        llvm::Value* codeGen_initial(Json::Value json_node);
        /*
         * codeGen_StmtsOp:
         * returns last llvm::Value from the array of simple statements.
         */
        llvm::Value* codeGen_StmtsOp(Json::Value json_node);
        llvm::Value* codeGen_ExprOp(Json::Value json_node);
        llvm::Value* codeGen_VarDef(Json::Value json_node);
        llvm::Value* codeGen_BinOp(Json::Value json_node);
        llvm::Value* codeGen_AtomOp(Json::Value json_node);
        llvm::Value* codeGen_ForOp(Json::Value json_node);
        llvm::Value* codeGen_WhileOp(Json::Value json_node);
        llvm::Value* codeGen_IfOp(Json::Value json_node);
        llvm::Value* codeGen_FuncDef(Json::Value json_node);
        llvm::Value* codeGen_TypedArg(Json::Value json_node);
        llvm::Value* codeGen_ReturnOp(Json::Value json_node);

        Json::Value generateFromJson(std::string json_string);
        void dumpIR();
        llvm::Module* getModule();
        llvm::LLVMContext* getContext();
};

#endif
