#ifndef SCOPE_HELPER_H
#define SCOPE_HELPER_H

#include <map>
#include "llvm/IR/Value.h"

class ScopeNode
{
    public:
        enum class ScopeType
        {
            TOP_SCOPE,
            FUNC_SCOPE,
            SIMPLE_SCOPE,
        };
        ScopeNode(ScopeType scopeType,
                ScopeNode* parent = nullptr, 
                std::map<std::string, llvm::Value *>* namedVals = nullptr);
        ~ScopeNode();
        ScopeNode* getParent();
        void setParent(ScopeNode* parent);
        void setNamedVal(std::string name, llvm::Value* value);
        llvm::Value* getNamedVal(std::string name);
        void setBlock(llvm::BasicBlock* block);
        ScopeType getScopeType();
    private:
        std::map<std::string, llvm::Value *>* namedVals;
        ScopeNode* parent;
        llvm::BasicBlock* block;
        ScopeType scopeType;
};

class ScopeHelper
{
    public:
       ScopeHelper(); 
       ~ScopeHelper(); 
       void pushScope(ScopeNode::ScopeType scopeType);
       void popScope();
       void setNamedVal(std::string name, llvm::Value* value, bool isDecl);
       llvm::Value* getNamedVal(std::string name, bool walkScopes);
       void setBlockOnCurrScope(llvm::BasicBlock* block);
       //getNearestScopeOfType returns nullptr if no scope of appropriate type is found
       ScopeNode* getNearestScopeOfType(ScopeNode::ScopeType scopeType);
    private:
        ScopeNode* parentScope;
        ScopeNode* currScope;
};

#endif
