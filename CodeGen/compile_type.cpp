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

bool CompileType::isGeneric()
{
    return genericsList.size() != 0;
}

std::vector<CompileType*>* CompileType::getGenericsList()
{
    return &genericsList;
}

void CompileType::insertGenericsList(CompileType* compileType)
{
    genericsList.push_back(compileType);
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

void CompileVal::insertGenericType(CompileType* compileType)
{
    this->compileType.insertGenericsList(compileType);
}

llvm::Value* CompileVal::getRawValue()
{
    return rawValue;
}
