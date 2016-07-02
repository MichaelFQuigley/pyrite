#ifndef COMPILE_TYPE_H
#define COMPILE_TYPE_H

#include <list>
#include <string>

#include "llvm/IR/Value.h"

class CompileType
{
    private:
        std::string typeName;
        std::list<CompileType> genericsList;
    public:
        CompileType(std::string typeName);
        std::string getTypeName();
        //isGeneric: returns true if this type has generic types within it
        bool isGeneric();
        std::list<CompileType> getGenericsList();
        //insertGenericsList: adds element to back of generics list
        void insertGenericsList(CompileType compileType);
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
        llvm::Value* getRawValue();
};

#endif
