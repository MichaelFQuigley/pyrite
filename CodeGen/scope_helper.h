#ifndef SCOPE_HELPER_H
#define SCOPE_HELPER_H

#include <map>
#include "llvm/IR/Value.h"

class ScopeNode
{
    public:
        ScopeNode(bool isFunctionScope=false,
                ScopeNode* parent = nullptr, 
                std::map<std::string, llvm::Value *>* namedVals = nullptr);
        ~ScopeNode();
        ScopeNode* getParent();
        void setParent(ScopeNode* parent);
        void setNamedVal(std::string name, llvm::Value* value);
        llvm::Value* getNamedVal(std::string name);
    private:
        std::map<std::string, llvm::Value *>* namedVals;
        ScopeNode* parent;
        bool isFunctionScope;
};

class ScopeHelper
{
    public:
       ScopeHelper(); 
       ~ScopeHelper(); 
       void pushScope(bool isFunctionScope = false);
       void popScope();
       void setNamedVal(std::string name, llvm::Value* value, bool isDecl);
       llvm::Value* getNamedVal(std::string name, bool walkScopes);
    private:
        ScopeNode* parentScope;
        ScopeNode* currScope;
};

#endif
