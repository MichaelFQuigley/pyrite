#include "scope_helper.h"
#include "codegen_util.h"

//ScopeNode
ScopeNode::ScopeNode(ScopeNode::ScopeType scopeType,
        ScopeNode* parent, 
        std::map<std::string, llvm::Value *>* namedVals)
{
    this->parent = parent;

    if( namedVals == nullptr )
    {
        this->namedVals = new std::map<std::string, llvm::Value *>();
    }
    else
    {
        this->namedVals = namedVals;
    }

    this->scopeType = scopeType;
}

ScopeNode::~ScopeNode()
{
    delete namedVals;
}

//ScopeNode getters/setters
ScopeNode* ScopeNode::getParent() { return parent; }

void ScopeNode::setParent(ScopeNode* parent) { this->parent = parent; }

void ScopeNode::setNamedVal(std::string name, llvm::Value* value) { (*namedVals)[name] = value; }

llvm::Value* ScopeNode::getNamedVal(std::string name) { return (*namedVals)[name]; }

void ScopeNode::setBlock(llvm::BasicBlock* block) { this->block = block; }

ScopeNode::ScopeType ScopeNode::getScopeType() { return scopeType; }

//ScopeHelper
ScopeHelper::ScopeHelper()
{
    parentScope = new ScopeNode(ScopeNode::ScopeType::TOP_SCOPE);
    currScope   = parentScope;
}

ScopeHelper::~ScopeHelper()
{

}

void ScopeHelper::pushScope(ScopeNode::ScopeType scopeType)
{
    ScopeNode* scopeNode = new ScopeNode(scopeType);
    scopeNode->setParent(currScope);
    currScope            = scopeNode;
}

void ScopeHelper::popScope()
{
    ScopeNode* oldScope = currScope;
    currScope           = currScope->getParent();
    delete oldScope;
}

void ScopeHelper::setNamedVal(std::string name, llvm::Value* value, bool isDecl)
{
    if( isDecl )
    {
        GEN_ASSERT(currScope->getNamedVal(name) == nullptr, 
                    "Variable already declared in scope.");
        currScope->setNamedVal(name, value);
    }
    else
    {
        ScopeNode* tempScope = currScope;

        while( tempScope != nullptr )
        {
            if( tempScope->getNamedVal(name) != nullptr )
            {
                tempScope->setNamedVal(name, value);
                return;
            }

            tempScope = tempScope->getParent();
        }

        GEN_ASSERT(false, "Undeclared variable.");
    }
}

llvm::Value* ScopeHelper::getNamedVal(std::string name, bool walkScopes)
{
    if( walkScopes )
    {
        ScopeNode* tempScope = currScope;

        while( tempScope != nullptr )
        {
            llvm::Value* tempNamedVal = tempScope->getNamedVal(name);

            if( tempNamedVal != nullptr )
            {
                return tempNamedVal;
            }
            tempScope = tempScope->getParent();
        }

        return nullptr;
    }
    else
    {
        return currScope->getNamedVal(name);
    }
}

void ScopeHelper::setBlockOnCurrScope(llvm::BasicBlock* block)
{
    currScope->setBlock(block);
}

ScopeNode* ScopeHelper::getNearestScopeOfType(ScopeNode::ScopeType scopeType)
{
    ScopeNode* tempScope = currScope;

    while( tempScope != nullptr && tempScope->getScopeType() != scopeType )
    {
        continue;
    }

    return nullptr;
}

