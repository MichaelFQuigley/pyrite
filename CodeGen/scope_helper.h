#ifndef SCOPE_HELPER_H
#define SCOPE_HELPER_H

/*
 * scope_helper manages scope of variables. The scope is a lexical scope.
 */

#include <map>
#include "llvm/IR/Value.h"
#include "compile_type.h"

class ScopeNode {
 public:
  enum class ScopeType {
    TOP_SCOPE,
    FUNC_SCOPE,
    SIMPLE_SCOPE,
  };
  ScopeNode(ScopeType scopeType, ScopeNode* parent = nullptr,
            std::map<std::string, std::tuple<uint64_t, CompileVal*>*>*
                namedVals = nullptr);

  ~ScopeNode();
  ScopeNode* getParent();
  void setParent(ScopeNode* parent);
  void setNamedVal(std::string name, CompileVal* value, uint64_t index);
  CompileVal* getNamedVal(std::string name);
  uint64_t getNamedValInd(std::string name);
  void setBlock(llvm::BasicBlock* block);
  bool isVoidReturn();
  void setFuncScopeRetVoid(bool isVoid);
  ScopeType getScopeType();
  uint64_t getNumNamedVarsInScope();
  void incFuncNamedVars();

 private:
  // namedVals: key = var name,
  // value = tuple(order in which named val was created, val)
  std::map<std::string, std::tuple<uint64_t, CompileVal*>*>* namedVals;
  ScopeNode* parent;
  llvm::BasicBlock* block;
  ScopeType scopeType;
  bool funcScopeRetVoid;
  uint64_t numNamedVarsInScope;
};

class ScopeHelper {
 public:
  ScopeHelper();
  ~ScopeHelper();
  void pushScope(ScopeNode::ScopeType scopeType, bool funcScopeRetVoid = false);
  void popScope();
  void setNamedVal(std::string name, CompileVal* value, bool isDecl);
  CompileVal* getNamedVal(std::string name, bool walkScopes);
  uint64_t getNamedValInd(std::string name);
  void setBlockOnCurrScope(llvm::BasicBlock* block);
  /*getNearestScopeOfType:
   * returns nullptr if no scope of appropriate type is found
   */
  ScopeNode* getNearestScopeOfType(ScopeNode::ScopeType scopeType);
  bool parentFuncReturnsVoid();
  void setParentFuncReturnsVoid(bool isVoid);
  /*getNumNamedVarsSinceFunc:
   * returns number of named variables that have been declared starting
   * from the nearest scope of type FUNC_SCOPE
   */
  uint64_t getNumNamedVarsSinceFunc();
  /*
   * incFuncNamedVars:
   * Increments the number of named variables that occur in
   * the current function scope.
   */
  void incFuncNamedVars();
  ScopeNode::ScopeType getCurrScopeType();

 private:
  ScopeNode* parentScope;
  ScopeNode* currScope;
};

#endif
