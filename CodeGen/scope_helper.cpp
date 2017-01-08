#include <cassert>

#include "scope_helper.h"
#include "codegen_util.h"

namespace codegen {

// ScopeNode
ScopeNode::ScopeNode(
    ScopeNode::ScopeType scopeType, ScopeNode *parent,
    std::map<std::string, std::tuple<uint64_t, CompileVal *> *> *namedVals) {
  this->parent = parent;
  numNamedVarsInScope = 0;

  if (namedVals == nullptr) {
    this->namedVals =
        new std::map<std::string, std::tuple<uint64_t, CompileVal *> *>();
  } else {
    this->namedVals = namedVals;
  }

  this->scopeType = scopeType;
}

ScopeNode::~ScopeNode() { delete namedVals; }

// ScopeNode getters/setters
ScopeNode *ScopeNode::getParent() { return parent; }

void ScopeNode::setParent(ScopeNode *parent) { this->parent = parent; }

void ScopeNode::setNamedVal(std::string name, CompileVal *value,
                            uint64_t index) {
  (*namedVals)[name] = new std::tuple<uint64_t, CompileVal *>(index, value);
}

CompileVal *ScopeNode::getNamedVal(std::string name) {
  if ((*namedVals)[name] == nullptr) {
    return nullptr;
  } else {
    return std::get<1>(*(*namedVals)[name]);
  }
}

uint64_t ScopeNode::getNamedValInd(std::string name) {
  GEN_ASSERT(getNamedVal(name) != nullptr, "Undeclared variable!");
  return std::get<0>(*(*namedVals)[name]);
}

void ScopeNode::setBlock(llvm::BasicBlock *block) { this->block = block; }

ScopeNode::ScopeType ScopeNode::getScopeType() { return scopeType; }

uint64_t ScopeNode::getNumNamedVarsInScope() { return numNamedVarsInScope; }

void ScopeNode::incFuncNamedVars() { numNamedVarsInScope++; }

// ScopeHelper
ScopeHelper::ScopeHelper() {
  parentScope = new ScopeNode(ScopeNode::ScopeType::TOP_SCOPE);
  currScope = parentScope;
}

ScopeHelper::~ScopeHelper() {}

void ScopeHelper::pushScope(ScopeNode::ScopeType scopeType) {
  ScopeNode *scopeNode = new ScopeNode(scopeType);
  scopeNode->setParent(currScope);
  currScope = scopeNode;
}

void ScopeHelper::popScope() {
  ScopeNode *oldScope = currScope;
  currScope = currScope->getParent();
  delete oldScope;
}

void ScopeHelper::setNamedVal(std::string name, CompileVal *value,
                              bool isDecl) {
  if (isDecl) {
    GEN_ASSERT(currScope->getNamedVal(name) == nullptr,
               "Variable " + name + " already declared in scope.");
    uint64_t numNamedVarsSinceFunc = getNumNamedVarsSinceFunc();
    currScope->setNamedVal(name, value, numNamedVarsSinceFunc);
    // Only increment gc vars for non-function types since memory that
    // is pointed to by a Function variable is not dynamically allocated.
    if (!value->getCompileType()->isFunctionType()) {
      incFuncNamedVars();
    }
  } else {
    ScopeNode *tempScope = currScope;

    while (tempScope != nullptr) {
      if (tempScope->getNamedVal(name) != nullptr) {
        uint64_t index = tempScope->getNamedValInd(name);
        tempScope->setNamedVal(name, value, index);
        return;
      }

      tempScope = tempScope->getParent();
    }

    GEN_ASSERT(false, "Undeclared variable.");
  }
}

CompileVal *ScopeHelper::getNamedVal(std::string name, bool walkScopes) {
  if (walkScopes) {
    ScopeNode *tempScope = currScope;

    while (tempScope != nullptr) {
      CompileVal *tempNamedVal = tempScope->getNamedVal(name);

      if (tempNamedVal != nullptr) {
        return tempNamedVal;
      }
      tempScope = tempScope->getParent();
    }

    return nullptr;
  } else {
    return currScope->getNamedVal(name);
  }
}

uint64_t ScopeHelper::getNamedValInd(std::string name) {
  ScopeNode *tempScope = currScope;

  while (tempScope != nullptr) {
    CompileVal *tempNamedVal = tempScope->getNamedVal(name);

    if (tempNamedVal != nullptr) {
      return tempScope->getNamedValInd(name);
    }
    tempScope = tempScope->getParent();
  }
  GEN_ASSERT(false, "Undelcared variable!");

  return 0;
}

void ScopeHelper::setBlockOnCurrScope(llvm::BasicBlock *block) {
  currScope->setBlock(block);
}

ScopeNode *ScopeHelper::getNearestScopeOfType(ScopeNode::ScopeType scopeType) {
  ScopeNode *tempScope = currScope;

  while (tempScope != nullptr && tempScope->getScopeType() != scopeType) {
    tempScope = tempScope->getParent();
  }

  return tempScope;
}

uint64_t ScopeHelper::getNumNamedVarsSinceFunc() {
  ScopeNode *tempScope =
      getNearestScopeOfType(ScopeNode::ScopeType::FUNC_SCOPE);
  // if not null, the we are at provided scope type
  if (tempScope != nullptr) {
    return tempScope->getNumNamedVarsInScope();
  } else {
    return 0;
  }
}

void ScopeHelper::incFuncNamedVars() {
  ScopeNode *tempScope =
      getNearestScopeOfType(ScopeNode::ScopeType::FUNC_SCOPE);

  if (tempScope != nullptr) {
    tempScope->incFuncNamedVars();
  }
}

ScopeNode::ScopeType ScopeHelper::getCurrScopeType() {
  return currScope->getScopeType();
}

}  // namespace codegen
