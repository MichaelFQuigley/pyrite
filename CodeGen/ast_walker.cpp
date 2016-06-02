#include <iostream>
#include <cstdlib>
#include "ast_walker.h"

using namespace std;

bool AstWalker::json_node_has(Json::Value json_node, std::string name, Json::Value* out_node)
{
    if( json_node[name] != Json::nullValue )
    {
           *out_node = json_node[name];
        return true;
    }

    return false;
}

llvm::Value* AstWalker::codeGen_StmtsOp(Json::Value json_node)
{
    llvm::Value* result = nullptr;
    for( unsigned i = 0; i < json_node.size(); i++ )
    {
        result = codeGen_initial(json_node[i]);
    }

    return result;
}

llvm::Value* AstWalker::codeGen_FuncProto(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_ExprOp(Json::Value json_node){ return codeGen_initial(json_node); }

llvm::Value* AstWalker::codeGen_VarDef(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_BinOp(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_AtomOp(Json::Value json_node) { 
    Json::Value val_node = Json::nullValue;

    if( json_node_has(json_node, "Lit", &val_node) ) 
    {
        std::string lit_str = val_node.asString();
        std::string lit_val = lit_str.substr(1);
        char lit_type = lit_str[0];

        double double_val;
        long int_val;
        switch(lit_type)
        {
            case 'f':
                double_val = stod(lit_val);
                cout << to_string(double_val) << endl;
                break;
            case 'i':
                int_val = stol(lit_val);
                cout << to_string(int_val) << endl;
                break;
            default:
                break;
        }
    }
    else if( json_node_has(json_node, "FCall", &val_node) ) {}
    else if( json_node_has(json_node, "Variable", &val_node) ) {}
    else
    {
        cout << "Unimplemented atom value." << endl;
    }
    return nullptr;
}

llvm::Value* AstWalker::codeGen_LoopOp(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_IfOp(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_FuncDef(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_TypedArg(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::AstWalker::codeGen_initial(Json::Value json_node)
{
    TRY_NODE(json_node, StmtsOp);
    TRY_NODE(json_node, FuncProto);
    TRY_NODE(json_node, ExprOp);
    TRY_NODE(json_node, VarDef);
    TRY_NODE(json_node, BinOp);
    TRY_NODE(json_node, AtomOp);
    TRY_NODE(json_node, LoopOp);
    TRY_NODE(json_node, IfOp);
    TRY_NODE(json_node, FuncDef);
    TRY_NODE(json_node, TypedArg);

    //If none of the TRY_NODE blocks returned anything, then we have an unimplemented ast node.
    std::cout << "Unimplemented node type in code generator" << std::endl;

    return nullptr;
}



Json::Value AstWalker::generateFromJson(std::string json_string)
{
    Json::Reader reader;
    Json::Value  json_root;
    reader.parse(json_string, json_root);
    return json_root;
}



int main()
{
    AstWalker* ast = new AstWalker();
    std::string json_string;

    if(std::cin) 
    {
        getline(std::cin, json_string);
        ast->codeGen_initial(ast->generateFromJson(json_string));
    }
}
