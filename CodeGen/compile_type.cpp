#include <iostream>
#include "compile_type.h"

CompileType::CompileType(std::string typeName)
{
    this->typeName = typeName;
}
/*
CompileType::CompileType(CompileType* compileType)
{
    this->typeName = compileType->getTypeName();
    this->genericsList = compileType;
}
*/
std::string CompileType::getTypeName() { return typeName; }

bool CompileType::isArgument()
{
    return genericsList.size() != 0;
}

std::vector<CompileType*>* CompileType::getArgumentsList()
{
    return &genericsList;
}

void CompileType::insertArgumentsList(CompileType* compileType)
{
    genericsList.push_back(compileType);
}

void CompileType::setArgumentsList(std::vector<CompileType*>* argsList)
{
    genericsList = *argsList;
}


CompileFunc::CompileFunc(CompileType* retType, std::vector<CompileType*>* arguments) 
{
    std::cout << arguments->size() << std::endl;
    this->arguments = arguments;
    this->retType   = retType;
}

CompileType* CompileFunc::getRetType()
{
    return retType;
}

std::vector<CompileType*>* CompileFunc::getArguments()
{
    return arguments;
}



CompileVal::CompileVal(llvm::Value* rawValue, std::string typeName) : compileType(typeName)
{
    this->rawValue = rawValue;
}

CompileVal::CompileVal(llvm::Value* rawValue, CompileType* compileType) : compileType(*compileType)
{
    this->rawValue = rawValue;
}

CompileType* CompileVal::getCompileType()
{
    return &compileType;
}

void CompileVal::setCompileType(CompileType* compileType)
{
    this->compileType = *compileType;
}

void CompileVal::insertArgumentType(CompileType* compileType)
{
    this->compileType.insertArgumentsList(compileType);
}

void CompileVal::setArgumentsList(std::vector<CompileType*>* argsList)
{
    this->compileType.setArgumentsList(argsList);
}


llvm::Value* CompileVal::getRawValue()
{
    return rawValue;
}
