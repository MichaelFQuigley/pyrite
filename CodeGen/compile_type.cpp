#include <iostream>
#include <stdexcept>
#include "compile_type.h"

CompileType::CompileType(std::string typeName)
{
    this->typeName = typeName;
}

CompileType::CompileType(CommonType commonType)
{
    this->typeName = CompileType::getCommonTypeName(commonType);
}

std::string CompileType::getCommonTypeName(CommonType commonType)
{
    switch( commonType )
    {
        case CompileType::CommonType::BOOL:
            return "Bool";
        case CompileType::CommonType::FLOAT:
            return "Float";
        case CompileType::CommonType::FUNCTION:
            return "Function";
        case CompileType::CommonType::INT:
            return "Int";
        case CompileType::CommonType::LIST:
            return "List";
        case CompileType::CommonType::STRING:
            return "String";
        case CompileType::CommonType::VOID:
            return "Void";
        default:
            throw std::runtime_error("Unknown common type!");
    }
}

bool CompileType::isVoidType(std::string typeName)
{
    return CompileType::getCommonTypeName(CompileType::CommonType::VOID)
           == typeName;
}

bool CompileType::isVoidType(CompileType* compileType)
{
    return CompileType::getCommonTypeName(CompileType::CommonType::VOID)
           == compileType->getTypeName();
}

bool CompileType::isVoidType(CommonType commonType)
{
    return CompileType::CommonType::VOID == commonType;
}

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

bool CompileType::isEqualToType(CompileType* testType)
{
    if( testType == nullptr )
    {
        return false;
    }
    // Base typename should be the same
    if( this->typeName != testType->typeName )
    {
        return false;
    }
    std::vector<CompileType*>* argsTypeA = this->getArgumentsList();
    std::vector<CompileType*>* argsTypeB = testType->getArgumentsList();
    if( argsTypeA->size() != argsTypeB->size() )
    {
        return false;
    }
    for(int i = 0; i < argsTypeA->size(); i++)
    {
        if( !((*argsTypeA)[i])->isEqualToType((*argsTypeB)[i]) )
        {
            return false;
        }
    }

    return true; 
}

bool CompileType::isCompatibleWithType(CompileType* incompleteType)
{
    if( incompleteType == nullptr )
    {
        return true;
    }

    if( incompleteType->typeName == this->typeName )
    {
        std::vector<CompileType*>* completeTypeArgs   = this->getArgumentsList();
        std::vector<CompileType*>* incompleteTypeArgs = incompleteType->getArgumentsList();
        size_t completeTypeNumArgs                    = completeTypeArgs->size();
        size_t incompleteTypeNumArgs                  = incompleteTypeArgs->size();

        // Function types must have same number of arguments.
        if( (this->typeName == CompileType::getCommonTypeName(CompileType::CommonType::FUNCTION)
             && (completeTypeNumArgs != incompleteTypeNumArgs))
            // Incomplete type should not be more complete than complete type.
            || (incompleteTypeNumArgs > completeTypeNumArgs) )
        {
            return false;
        }

        bool result = true;
        for(int i = 0; i < incompleteTypeNumArgs; i++)
        {
            std::cout << ((*completeTypeArgs)[i])->typeName << std::endl;
            std::cout << ((*incompleteTypeArgs)[i])->typeName << std::endl;
            result &= ((*completeTypeArgs)[i])->isCompatibleWithType((*incompleteTypeArgs)[i]);
        }
           
        return result;
    }
    else
    {
        return false;
    }
}

CompileType* CompileType::getFunctionReturnType()
{
    std::vector<CompileType*>* arguments = this->getArgumentsList();

    //last argument in CompileType is return type
    return (*arguments)[arguments->size() - 1];
}


CompileVal::CompileVal(llvm::Value* rawValue, std::string typeName) : compileType(typeName)
{
    this->rawValue = rawValue;
}

CompileVal::CompileVal(llvm::Value* rawValue, CompileType* compileType) : compileType(*compileType)
{
    this->rawValue = rawValue;
}

CompileVal::CompileVal(llvm::Value* rawValue,
                       CompileType::CommonType commonType) : compileType(commonType)
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

bool CompileVal::typesAreEqual(CompileVal* valB)
{
    return this->getCompileType()->isEqualToType(valB->getCompileType());
}

