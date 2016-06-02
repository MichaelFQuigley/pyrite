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

#include "ast_node.h"

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
        bool json_node_has(Json::Value json_node, std::string name, Json::Value* out_node);
    public:
        llvm::Value* codeGen_initial(Json::Value json_node);
        /*
         * codeGen_StmtsOp:
         * returns last llvm::Value from the array of simple statements.
         */
        llvm::Value* codeGen_StmtsOp(Json::Value json_node);
        llvm::Value* codeGen_FuncProto(Json::Value json_node);
        llvm::Value* codeGen_ExprOp(Json::Value json_node);
        llvm::Value* codeGen_VarDef(Json::Value json_node);
        llvm::Value* codeGen_BinOp(Json::Value json_node);
        llvm::Value* codeGen_AtomOp(Json::Value json_node);
        llvm::Value* codeGen_LoopOp(Json::Value json_node);
        llvm::Value* codeGen_IfOp(Json::Value json_node);
        llvm::Value* codeGen_FuncDef(Json::Value json_node);
        llvm::Value* codeGen_TypedArg(Json::Value json_node);

        Json::Value generateFromJson(std::string json_string);
};
