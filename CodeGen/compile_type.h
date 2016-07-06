#ifndef COMPILE_TYPE_H
#define COMPILE_TYPE_H

#include <vector>
#include <string>

#include "llvm/IR/Value.h"

class CompileType
{
    private:
        std::string typeName;
        std::vector<CompileType*> genericsList;
    public:
        CompileType(std::string typeName);
        std::string getTypeName();
        //isArgument: returns true if this type has generic types within it
        bool isArgument();
        std::vector<CompileType*>* getArgumentsList();
        //insertArgumentsList: adds element to back of generics vector
        void insertArgumentsList(CompileType* compileType);
        void setArgumentsList(std::vector<CompileType*>* argsList);
};

class CompileFunc
{
    private:
        CompileType* retType;
        std::vector<CompileType*>* arguments;
    public:
        CompileFunc(CompileType* retType, std::vector<CompileType*>* arguments);
        CompileType* getRetType();
        std::vector<CompileType*>* getArguments();
};

class CompileVal
{
    private:
        llvm::Value* rawValue;
        CompileType  compileType;
    public:
        CompileVal(llvm::Value* rawValue, std::string typeName);
        CompileVal(llvm::Value* rawValue, CompileType* compileType);
        CompileType* getCompileType();
        void setCompileType(CompileType* compileType);
        void insertArgumentType(CompileType* compileType);
        void setArgumentsList(std::vector<CompileType*>* argsList);
        llvm::Value* getRawValue();
};

#endif
