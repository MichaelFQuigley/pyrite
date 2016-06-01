#include <iostream>
#include "ast_walker.h"

using namespace std;

void AstWalker::codeGen_stmts(Json::Value json_node)
{
}




void AstWalker::codeGen_initial(Json::Value json_node)
{
    if( json_node["StmtsOp"] != Json::nullValue ) 
    {
        codeGen_stmts(json_node["StmtsOp"]);
    }
    else 
    {
        std::cout << "Unimplemented node type in code generator" << std::endl;
        return;
    }
}



Json::Value AstWalker::generateFromJson(std::string json_string)
{
    Json::Reader reader;
    Json::Value  json_root;
    if( reader.parse(json_string, json_root) != Json::nullValue )
    {
        return json_root;
    }
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
