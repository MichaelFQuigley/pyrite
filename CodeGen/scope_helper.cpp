#include "scope_helper.h"
#include "codegen_util.h"

//ScopeNode
ScopeNode::ScopeNode(bool isFunctionScope,
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

    this->isFunctionScope = isFunctionScope;
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


//ScopeHelper
ScopeHelper::ScopeHelper()
{
    parentScope = new ScopeNode();
    currScope   = parentScope;
}

ScopeHelper::~ScopeHelper()
{

}

void ScopeHelper::pushScope(bool isFunctionScope)
{
    ScopeNode* scopeNode = new ScopeNode(isFunctionScope);
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
