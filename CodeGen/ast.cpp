#include <iostream>
#include "ast.h"

bool Ast::generateFromJson(std::string json_string)
{
    Json::Reader reader;
    std::cout << "here" << std::endl;    
    if( !reader.parse(json_string, root) )
        return false;

    std::cout << root << std::endl;    
    return true;
}



int main()
{
    Ast* ast = new Ast();
    std::string json_string;

    if(std::cin) 
    {
        getline(std::cin, json_string);
        ast->generateFromJson(json_string);
    }
}
