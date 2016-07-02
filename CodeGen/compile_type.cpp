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

std::list<CompileType> CompileType::getGenericsList()
{
    return genericsList;
}

void CompileType::insertGenericsList(CompileType compileType)
{
    genericsList.push_back(compileType);
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

llvm::Value* CompileVal::getRawValue()
{
    return rawValue;
}
