#include <iostream>
#include <cstdlib>
#include "ast_walker.h"

namespace codegen {

using namespace std;

AstWalker::AstWalker(
    llvm::Module *outputModule,
    std::map<std::string, CompileType *> const *currModuleTypes)
    : currContext(outputModule->getContext()), Builder(currContext) {
  currModule = outputModule;
  scopeHelper = new ScopeHelper();
  codeGenHelper =
      new CodeGenUtil(currModule, &currContext, Builder, *currModuleTypes);

  // TODO automate this process...
  CompileVal *printlnVal = new CompileVal(
      codeGenHelper->tryGetFunction("println"),
      new CompileType(CompileType::CommonType::FUNCTION), /*isBoxed=*/false);
  printlnVal->setArgumentsList(new std::vector<CompileType *>(
      {new CompileType(CompileType::CommonType::STRING),
       new CompileType(CompileType::CommonType::VOID)}));
  addFuncPtr("println", printlnVal);
}

void AstWalker::addFuncPtr(std::string funcName, CompileVal *func) {
  if (scopeHelper->getCurrScopeType() == ScopeNode::ScopeType::TOP_SCOPE) {
    createGlobalFunctionConst(funcName, func);
  } else {
    GEN_FAIL("Nested functions not implemented yet.");
  }
}

bool AstWalker::jsonNode_has(Json::Value &jsonNode, const std::string &name,
                             Json::Value *out_node) {
  if (jsonNode[name] != Json::nullValue) {
    *out_node = jsonNode[name];
    return true;
  }

  return false;
}

Json::Value AstWalker::jsonNodeGet(Json::Value &node, const std::string &name) {
  if (node[name] == Json::nullValue) {
    std::cout << node << std::endl;
    GEN_FAIL("No node element with name: " + name);
  }

  return node[name];
}

CompileVal *AstWalker::codeGen_StmtsOp(Json::Value &jsonNode) {
  CompileVal *result = nullptr;
  for (auto &stmt_node : jsonNode) {
    result = codeGen_initial(stmt_node);
  }

  return result;
}

void AstWalker::pushScope(ScopeNode::ScopeType scopeType) {
  scopeHelper->pushScope(scopeType);
}

void AstWalker::popScope() { scopeHelper->popScope(); }

CompileType *AstWalker::makeCompileType(Json::Value &jsonNode) {
  CompileType *result = nullptr;
  if (jsonNode["simple"] != Json::nullValue) {
    result = codeGenHelper->getCompileType(jsonNode["simple"].asString());
  } else if (jsonNode["list_type"] != Json::nullValue) {
    result = codeGenHelper->getCompileType(CompileType::CommonType::LIST);
    // Make list generic type concrete.
    result = CompileType::implementGeneric(
        CodeGenUtil::LIST_GENERIC_PARAM, makeCompileType(jsonNode["list_type"]),
        result);
  } else if (jsonNode["func_type"] != Json::nullValue)
    return makeFuncCompileType(jsonNode["func_type"]);
  else {
    cout << jsonNode << endl;
    GEN_FAIL("Unimplemented compile type for makeCompileType");
  }

  return result;
}

CompileType *AstWalker::makeFuncCompileType(Json::Value &jsonNode) {
  GEN_ASSERT(jsonNode["FuncProto"] != Json::nullValue,
             "FuncProto node must exist.");
  Json::Value &headerNode = jsonNode["FuncProto"];
  std::vector<CompileType *> *compileArgs = new std::vector<CompileType *>();

  for (Json::Value &val : headerNode["args"]) {
    CompileType *argType = makeCompileType(val["TypedArg"]["type"]);
    compileArgs->push_back(argType);
  }
  CompileType *compileRetType = makeCompileType(headerNode["ret_type"]);
  compileArgs->push_back(compileRetType);

  CompileType *result = new CompileType(CompileType::CommonType::FUNCTION);
  result->setArgumentsList(compileArgs);
  result->setParent(
      codeGenHelper->getCompileType(CompileType::CommonType::BASE));
  return result;
}

CompileVal *AstWalker::makeFuncProto(Json::Value &jsonNode) {
  std::vector<llvm::Type *> argsV;
  Json::Value &headerNode = jsonNode["header"];
  CompileType *resultType = makeFuncCompileType(headerNode);

  auto argTypes = resultType->getArgumentsList();
  for (int i = 0; i < headerNode["FuncProto"]["args"].size(); i++) {
    CompileType *argType = (*argTypes)[i];
    argsV.push_back(rawTypeFromCompileType(argType));
  }

  std::string funcName = jsonNode["name"].asString();
  llvm::Type *ret_type =
      rawTypeFromCompileType(resultType->getFunctionReturnType());
  llvm::FunctionType *funcProto =
      llvm::FunctionType::get(ret_type, argsV, false);
  llvm::Function *func = llvm::Function::Create(
      funcProto, llvm::Function::ExternalLinkage, funcName, currModule);
  int arg_index = 0;
  for (auto argI = func->arg_begin(); arg_index < ((int)func->arg_size());
       argI++, arg_index++) {
    argI->setName(headerNode["FuncProto"]["args"][arg_index]["TypedArg"]["name"]
                      .asString());
  }

  CompileVal *result = new CompileVal(func, resultType, /*isBoxed=*/false);
  return result;
}

CompileVal *AstWalker::codeGen_ExprOp(Json::Value &jsonNode) {
  return codeGen_initial(jsonNode);
}

CompileVal *AstWalker::codeGen_VarDef(Json::Value &jsonNode) {
  Json::Value tempNode;

  if (jsonNode_has(jsonNode, "definition", &tempNode)) {
    std::string varName = tempNode["var"]["TypedArg"]["name"].asString();
    CompileType *annotatedType =
        makeCompileType(tempNode["var"]["TypedArg"]["type"]);
    CompileVal *rhs_expr = codeGen_initial(tempNode["expr"]);
    GEN_ASSERT(rhs_expr != nullptr,
               "Invalid assignment to rhs of variable definition.");
    GEN_ASSERT(CompileType::lhsTypeCanReplaceRhsType(
                   annotatedType, rhs_expr->getCompileType()),
               "Annotated type " + annotatedType->getLongTypeName() +
                   " is not compatible with type on right hand side: " +
                   rhs_expr->getCompileType()->getLongTypeName());
    CompileVal *varVal = new CompileVal(rhs_expr->getRawValue(), annotatedType,
                                        rhs_expr->getIsBoxed());
    newVarInScope(varName, varVal);
  } else if (jsonNode_has(jsonNode, "definition_infer", &tempNode)) {
    std::string varName = tempNode["name"].asString();
    CompileVal *rhs_expr = codeGen_initial(tempNode["expr"]);
    GEN_ASSERT(rhs_expr != nullptr,
               "Invalid assignment to rhs of variable definition.");
    newVarInScope(varName, rhs_expr);
  } else if (jsonNode_has(jsonNode, "declaration", &tempNode)) {
    std::string varName = tempNode["TypedArg"]["name"].asString();
    CompileType *compileType = makeCompileType(tempNode["TypedArg"]["type"]);
    newVarInScope(varName, new CompileVal(nullptr, compileType), false);
  } else {
    GEN_FAIL("Invalid variable definition.");
  }

  return nullptr;
}

CompileVal *AstWalker::newVarInScope(const std::string &varName,
                                     CompileVal *value, bool is_definition) {
  value = codeGenHelper->boxIfNot(value);
  llvm::BasicBlock *originalBlock = Builder.GetInsertBlock();
  llvm::BasicBlock &func_block = originalBlock->getParent()->getEntryBlock();

  Builder.SetInsertPoint(&func_block);
  llvm::Value *allocaRes =
      Builder.CreateAlloca(rawTypeFromCompileType(value->getCompileType()));
  Builder.SetInsertPoint(originalBlock);

  if (is_definition) {
    Builder.CreateStore(value->getRawValue(), allocaRes);
  }

  CompileVal *result =
      new CompileVal(allocaRes, value->getCompileType(), value->getIsBoxed());
  scopeHelper->setNamedVal(varName, result, true);

  return result;
}

void AstWalker::handleAssignLhs(Json::Value &assignLhs, CompileVal *rhs) {
  Json::Value &lhs_val = assignLhs["Id"];
  std::string id_name = lhs_val["name"].asString();
  Json::Value &trailers = lhs_val["trailers"];
  CompileVal *result = nullptr;
  CompileVal *var_val = scopeHelper->getNamedVal(id_name, true);

  GEN_ASSERT(var_val != nullptr, "Variable " + id_name + " is undefined.");

  result = new CompileVal(Builder.CreateLoad(var_val->getRawValue()),
                          var_val->getCompileType(), var_val->getIsBoxed());
  if (!result->getIsBoxed()) {
    result = codeGenHelper->boxValue(result);
  }

  if (trailers.size() > 0) {
    Json::Value currNode;
    for (int i = 0; i < trailers.size(); i++) {
      if (jsonNode_has(trailers[i], "FCall", &currNode)) {
        GEN_ASSERT(i < trailers.size() - 1, "Function call is not assignable!");

        std::vector<CompileVal *> *fcallArgs = new std::vector<CompileVal *>();
        for (Json::Value &val : currNode["args"]) {
          fcallArgs->push_back(codeGen_initial(val));
        }

        result = createLangCall(result, *fcallArgs);
      } else if (jsonNode_has(trailers[i], "Index", &currNode)) {
        CompileVal *indexVal = codeGen_initial(currNode);

        if (i < trailers.size() - 1) {
          result = createClassMethodCall("get", result, {indexVal});
        } else {
          createClassMethodCall("set", result, {indexVal, rhs});
        }
      } else if (jsonNode_has(trailers[i], "Dot", &currNode)) {
        GEN_FAIL("Assignment to fields not implemented yet.");
      }
    }
  } else {  // Assign to value in result.
    uint64_t varIndex = scopeHelper->getNamedValInd(id_name);

    GEN_ASSERT(CompileType::lhsTypeCanReplaceRhsType(result->getCompileType(),
                                                     rhs->getCompileType()),
               "Type on rhs is not compatible with lhs in assignment of " +
                   var_val->getCompileType()->getLongTypeName() + " " +
                   id_name + ".");
    if (!rhs->getIsBoxed()) {
      rhs = codeGenHelper->boxValue(rhs);
    }
    Builder.CreateStore(rhs->getRawValue(), var_val->getRawValue());
    codeGenHelper->createNativeCall(
        "gc_set_named_var_in_scope",
        {Builder.CreateLoad(var_val->getRawValue()),
         codeGenHelper->getConstInt64(varIndex, false)});
  }
}

CompileVal *AstWalker::codeGen_BinOp(Json::Value &jsonNode) {
  std::string op = jsonNode["op"].asString();
  std::string opFuncPrefix = "";
  CompileVal *lhs = codeGen_initial(jsonNode["lhs"]);
  CompileVal *rhs = codeGen_initial(jsonNode["rhs"]);

  if (op == "=") {
    Json::Value lhs_node;
    if (jsonNode_has(jsonNode["lhs"], "AtomOp", &lhs_node)) {
      CompileVal *rhs_val = codeGen_initial(jsonNode["rhs"]);

      GEN_ASSERT(rhs_val != nullptr,
                 "Invalid expression on rhs of assignment.");

      handleAssignLhs(lhs_node, rhs_val);

      return rhs_val;
    } else {
      GEN_FAIL("Invalid lhs of assignment.");
    }
  }
  // If both sides are Int and at least one side is unboxed, use unboxed
  // instructions.
  if (lhs->getCompileType()->isIntType() &&
      lhs->getCompileType()->isIntType() &&
      (!lhs->getIsBoxed() || !rhs->getIsBoxed())) {
    lhs = codeGenHelper->unboxIfNot(lhs);
    rhs = codeGenHelper->unboxIfNot(rhs);

    return codeGenHelper->generateUnboxedIntBinOp(lhs, rhs, op);
  }
  if (op == "+") {
    opFuncPrefix = CodeGenUtil::ADD_FUNC;
  } else if (op == "-") {
    opFuncPrefix = CodeGenUtil::SUB_FUNC;
  } else if (op == "*") {
    opFuncPrefix = CodeGenUtil::MUL_FUNC;
  } else if (op == "/") {
    opFuncPrefix = CodeGenUtil::DIV_FUNC;
  } else if (op == "%") {
    opFuncPrefix = CodeGenUtil::MOD_FUNC;
  } else if (op == "&") {
    opFuncPrefix = CodeGenUtil::AND_FUNC;
  } else if (op == "|") {
    opFuncPrefix = CodeGenUtil::OR_FUNC;
  } else if (op == "^") {
    opFuncPrefix = CodeGenUtil::XOR_FUNC;
  } else if (op == "<") {
    opFuncPrefix = CodeGenUtil::CMPLT_FUNC;
  } else if (op == "<=") {
    opFuncPrefix = CodeGenUtil::CMPLE_FUNC;
  } else if (op == "==") {
    opFuncPrefix = CodeGenUtil::CMPEQ_FUNC;
  } else if (op == ">") {
    opFuncPrefix = CodeGenUtil::CMPGT_FUNC;
  } else if (op == ">=") {
    opFuncPrefix = CodeGenUtil::CMPGE_FUNC;
  } else if (op == "!=") {
    opFuncPrefix = CodeGenUtil::CMPNE_FUNC;
  } else {
    GEN_FAIL("Unimplemented operator type!");
  }

  return createClassMethodCall(opFuncPrefix, lhs, {rhs});
}

CompileVal *AstWalker::codeGen_AtomOp(Json::Value &jsonNode) {
  Json::Value val_node = Json::nullValue;

  if (jsonNode_has(jsonNode, "Lit", &val_node)) {
    std::string lit_str = val_node.asString();
    std::string lit_val = lit_str.substr(1);
    char lit_type = lit_str[0];

    double double_val;
    long int_val;
    // Literal types have a prefix specifying their kind.
    switch (lit_type) {
      // Float
      case 'f':
        double_val = stod(lit_val);
        return codeGenHelper->createLiteral(
            CompileType::CommonType::FLOAT,
            llvm::ConstantFP::get(currContext, llvm::APFloat(double_val)));
      // Int
      case 'i':
        int_val = stol(lit_val);
        return codeGenHelper->createLiteral(CompileType::CommonType::INT,
                                            Builder.getInt64(int_val));
      // String
      case 's':
        return codeGenHelper->createLiteral(
            CompileType::CommonType::STRING,
            Builder.CreateGlobalStringPtr(lit_val));
      // Bool
      case 'b':
        return codeGenHelper->createLiteral(CompileType::CommonType::BOOL,
                                            Builder.getInt1(lit_val == "true"));
      default:
        GEN_FAIL("Unimplemented literal type");
    }
  } else if (jsonNode_has(jsonNode, "Id", &val_node)) {
    std::string id_name = val_node["name"].asString();
    CompileVal *result = nullptr;

    CompileVal *var_val = scopeHelper->getNamedVal(id_name, true);

    GEN_ASSERT(var_val != nullptr, "Variable " + id_name + " is undefined.");

    result = new CompileVal(Builder.CreateLoad(var_val->getRawValue()),
                            var_val->getCompileType(),
                            /*isBoxed=*/var_val->getIsBoxed());
    return handleTrailers(val_node["trailers"], result);
  } else if (jsonNode_has(jsonNode, "ParenExpr", &val_node)) {
    return handleTrailers(val_node["trailers"],
                          codeGen_ExprOp(val_node["expr"]));
  } else if (jsonNode_has(jsonNode, "RangeOp", &val_node)) {
    CompileVal *start =
        codeGenHelper->boxIfNot(codeGen_AtomOp(val_node["start"]));
    CompileVal *step = codeGenHelper->boxIfNot(codeGenHelper->createLiteral(
        CompileType::CommonType::INT,
        llvm::ConstantInt::get(currContext, llvm::APInt(64, 1, true))));
    CompileVal *end = codeGenHelper->boxIfNot(codeGen_AtomOp(val_node["end"]));

    GEN_ASSERT(start->typesAreEqual(end),
               "Start and end type for range must be the same.");
    CompileType *iteratedType = start->getCompileType();
    CompileType *rangeType =
        codeGenHelper->getCompileType(iteratedType->getTypeName() + "Range");
    llvm::Value *init_RangeResult = codeGenHelper->createNativeCall(
        "init_" + rangeType->getTypeName(),
        {start->getRawValue(), step->getRawValue(), end->getRawValue()});
    return new CompileVal(init_RangeResult, rangeType);
  } else {
    GEN_FAIL("Unimplemented atom value.");
  }
  return nullptr;
}

CompileVal *AstWalker::handleTrailers(Json::Value &trailers,
                                      CompileVal *currVal) {
  Json::Value currNode;
  CompileVal *result = currVal;
  // containingObject is non-null when the previous trailer represented
  // an object access.
  CompileVal *containingObject = nullptr;
  for (auto &trailer : trailers) {
    if (jsonNode_has(trailer, "FCall", &currNode)) {
      std::vector<CompileVal *> *fcallArgs = new std::vector<CompileVal *>();
      if (containingObject != nullptr) {
        fcallArgs->push_back(containingObject);
      }
      for (Json::Value &val : currNode["args"]) {
        fcallArgs->push_back(codeGen_initial(val));
      }
      result = createLangCall(result, *fcallArgs);
      containingObject = nullptr;
    } else if (jsonNode_has(trailer, "Index", &currNode)) {
      CompileVal *indexVal = codeGen_initial(currNode);
      result = createLangCall(codeGenHelper->createVtableAccess(result, "get"),
                              {result, indexVal});
      containingObject = nullptr;
    } else if (jsonNode_has(trailer, "Dot", &currNode)) {
      int methodIndex = result->getCompileType()->getMethodIndex(
          currNode.asString(), nullptr);
      if (methodIndex == -1) {
        int fieldIndex = result->getCompileType()->getFieldIndex(
            currNode.asString(), nullptr);
        GEN_ASSERT(fieldIndex != -1,
                   "Unknown field or method '" + currNode.asString() +
                       "' in class " + result->getCompileType()->getTypeName());
        GEN_FAIL("Field access not implemented yet.");
      }
      containingObject = result;
      result = codeGenHelper->createVtableAccess(result, currNode.asString());
    }
  }
  return result;
}

llvm::BasicBlock *AstWalker::makeBasicBlock(const std::string &name) {
  return llvm::BasicBlock::Create(currContext, name,
                                  Builder.GetInsertBlock()->getParent());
}

CompileVal *AstWalker::createLangCall(CompileVal *func,
                                      const std::vector<CompileVal *> &argsV) {
  GEN_ASSERT(func->getCompileType()->isFunctionType(),
             "Error: Trying to call something that is not a function! (i.e. " +
                 func->getCompileType()->getLongTypeName() + ")");
  func = codeGenHelper->unboxIfNot(func);

  std::vector<llvm::Value *> nativeArgs;
  const std::vector<CompileType *> &funcProtoArgs =
      *(func->getCompileType()->getArgumentsList());
  CompileType const *retType = func->getCompileType()->getFunctionReturnType();

  GEN_ASSERT(argsV.size() == funcProtoArgs.size() - 1,
             string("Number of arguments in function call must match.\n") +
                 "Expected " + to_string(funcProtoArgs.size() - 1) +
                 " arguments, but received " + to_string(argsV.size()) +
                 "\n"
                 "in function " +
                 func->getCompileType()->getLongTypeName());

  for (int i = 0; i < argsV.size(); i++) {
    const CompileType *funcProtoArgType = funcProtoArgs[i]->isGenericType()
                                              ? funcProtoArgs[i]->getParent()
                                              : funcProtoArgs[i];
    GEN_ASSERT(
        CompileType::isTypeOrSubtype(argsV[i]->getCompileType(), funcProtoArgType),
        (std::string)"Types used in call to function do not match function prototype."
        + "\nExpected " + funcProtoArgType->getLongTypeName()
        + "\nGot " + argsV[i]->getCompileType()->getLongTypeName());
    CompileVal *param = argsV[i];
    param = codeGenHelper->boxIfNot(param);
    nativeArgs.push_back(param->getRawValue());
  }
  // Since functions are all stored as void stars, it must be casted to the
  // proper function type.
  llvm::Value *rawFunc = Builder.CreatePointerCast(
      func->getRawValue(),
      llvm::PointerType::getUnqual(CompileType::asRawFunctionType(
          func->getCompileType(), Builder.getInt8PtrTy(), currContext,
          false /* includeThisPointer */)));
  llvm::Value *retVal = Builder.CreateCall(rawFunc, nativeArgs);

  return new CompileVal(retVal, retType);
}

void AstWalker::createBoolCondBr(CompileVal *Bool, llvm::BasicBlock *trueBlock,
                                 llvm::BasicBlock *falseBlock) {
  GEN_ASSERT(Bool->getCompileType()->isBoolType(),
             "Expected type Bool, but got type: " +
                 Bool->getCompileType()->getLongTypeName());
  llvm::Value *rawBool = codeGenHelper->unboxIfNot(Bool)->getRawValue();

  Builder.CreateCondBr(rawBool, trueBlock, falseBlock);
}

CompileVal *AstWalker::codeGen_ForOp(Json::Value &jsonNode) {
  llvm::BasicBlock *loopTop = makeBasicBlock("loopTop");
  llvm::BasicBlock *loopBody =
      llvm::BasicBlock::Create(currContext, "loopBody");
  llvm::BasicBlock *loopBottom =
      llvm::BasicBlock::Create(currContext, "loopBottom");

  pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);

  CompileVal *itt = codeGen_AtomOp(jsonNode["itt"]);
  std::string loopVarName = jsonNode["loop_var"].asString();

  startForLoop(itt, loopVarName, loopTop, loopBody, loopBottom);
  codeGen_initial(jsonNode["body"]);
  endForLoop(itt, loopVarName, loopTop, loopBottom);

  popScope();

  return nullptr;
}

void AstWalker::startForLoop(CompileVal *iteratorVal,
                             const std::string &loopVarName,
                             llvm::BasicBlock *loopTop,
                             llvm::BasicBlock *loopBody,
                             llvm::BasicBlock *loopBottom) {
  // Call to indexVariable = itt.begin()
  newVarInScope(loopVarName, createClassMethodCall(CodeGenUtil::ITERATOR_BEGIN,
                                                   iteratorVal));
  Builder.CreateBr(loopTop);
  Builder.SetInsertPoint(loopTop);

  CompileVal *hasNext =
      createClassMethodCall(CodeGenUtil::ITERATOR_HASNEXT, iteratorVal);
  createBoolCondBr(hasNext, loopBody, loopBottom);
  // loop body
  startBlock(loopBody);
  codeGenHelper->createNativeCall("gc_push_loop_scope", {});
}

void AstWalker::endForLoop(CompileVal *iteratorVal,
                           const std::string &loopVarName,
                           llvm::BasicBlock *loopTop,
                           llvm::BasicBlock *loopBottom) {
  Builder.CreateStore(
      createClassMethodCall(CodeGenUtil::ITERATOR_NEXT, iteratorVal)
          ->getRawValue(),
      scopeHelper->getNamedVal(loopVarName, true)->getRawValue());

  codeGenHelper->createNativeCall("gc_pop_scope", {});
  Builder.CreateBr(loopTop);
  startBlock(loopBottom);
}

CompileVal *AstWalker::codeGen_WhileOp(Json::Value &jsonNode) {
  llvm::BasicBlock *loopTop = llvm::BasicBlock::Create(currContext, "whileTop");
  llvm::BasicBlock *loopBody =
      llvm::BasicBlock::Create(currContext, "whileBody");
  llvm::BasicBlock *loopBottom =
      llvm::BasicBlock::Create(currContext, "whileBottom");

  pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);

  Builder.CreateBr(loopTop);
  startBlock(loopTop);

  CompileVal *header = codeGen_initial(jsonNode["header"]);
  createBoolCondBr(header, loopBody, loopBottom);

  startBlock(loopBody);
  codeGen_initial(jsonNode["body"]);

  Builder.CreateBr(loopTop);
  startBlock(loopBottom);

  popScope();

  return nullptr;
}

void AstWalker::startBlock(llvm::BasicBlock *block) {
  llvm::Function *currFunction = Builder.GetInsertBlock()->getParent();
  currFunction->getBasicBlockList().push_back(block);
  Builder.SetInsertPoint(block);
}

CompileVal *AstWalker::codeGen_IfOp(Json::Value &jsonNode) {
  CompileVal *test = codeGen_initial(jsonNode["test"]);

  llvm::BasicBlock *ifTrue = makeBasicBlock("ifTrue");
  llvm::BasicBlock *endIf;

  CompileVal *result = nullptr;
  // check to see if there is just an if block without an else
  if (jsonNode["bodies"].size() == 1) {
    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
    endIf = llvm::BasicBlock::Create(currContext, "endif");
    createBoolCondBr(test, ifTrue, endIf);
    Builder.SetInsertPoint(ifTrue);
    codeGen_initial(jsonNode["bodies"][0]);
    Builder.CreateBr(endIf);
    startBlock(endIf);
    popScope();
  }
  // else block (size == 2)
  else {
    llvm::BasicBlock *ifFalse =
        llvm::BasicBlock::Create(currContext, "ifFalse");
    endIf = llvm::BasicBlock::Create(currContext, "endif");
    createBoolCondBr(test, ifTrue, ifFalse);
    // if true codegen
    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
    Builder.SetInsertPoint(ifTrue);
    CompileVal *ifTrueLastStmt = codeGen_initial(jsonNode["bodies"][0]);
    popScope();
    // if false codegen
    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
    Builder.CreateBr(endIf);
    ifTrue = Builder.GetInsertBlock();
    startBlock(ifFalse);
    CompileVal *ifFalseLastStmt = codeGen_initial(jsonNode["bodies"][1]);
    // Make phi inputs either both boxed or both unboxed.
    if (ifTrueLastStmt->getIsBoxed() && !ifFalseLastStmt->getIsBoxed()) {
      ifFalseLastStmt = codeGenHelper->boxValue(ifFalseLastStmt);
    } else if (!ifTrueLastStmt->getIsBoxed() && ifFalseLastStmt->getIsBoxed()) {
      ifFalseLastStmt = codeGenHelper->unboxValue(ifFalseLastStmt);
    }
    Builder.CreateBr(endIf);
    ifFalse = Builder.GetInsertBlock();
    startBlock(endIf);
    popScope();
    // phi node
    if (ifTrueLastStmt != nullptr && ifFalseLastStmt != nullptr &&
        !(CompileType::isVoidType(ifTrueLastStmt->getCompileType())) &&
        !(CompileType::isVoidType(ifFalseLastStmt->getCompileType()))) {
      llvm::PHINode *phi = Builder.CreatePHI(
          ifTrueLastStmt->getRawValue()->getType(), 2, "ifPhi");

      phi->addIncoming(ifTrueLastStmt->getRawValue(), ifTrue);
      phi->addIncoming(ifFalseLastStmt->getRawValue(), ifFalse);
      result = new CompileVal(phi, ifTrueLastStmt->getCompileType(),
                              ifTrueLastStmt->getIsBoxed());
    } else {
      result = nullptr;
    }
  }

  return result;
}

llvm::Value *AstWalker::createGlobalFunctionConst(const std::string &funcName,
                                                  CompileVal *func) {
  llvm::Function *initValue =
      static_cast<llvm::Function *>(func->getRawValue());
  ScopeNode *globalScope =
      scopeHelper->getNearestScopeOfType(ScopeNode::ScopeType::TOP_SCOPE);

  llvm::GlobalVariable *result = new llvm::GlobalVariable(
      *currModule, func->getRawValue()->getType(), /*isConstant=*/true,
      llvm::GlobalValue::LinkageTypes::PrivateLinkage, initValue, funcName);
  // Cast from function type to void* type.
  llvm::Value *castedResult = Builder.CreatePointerCast(
      result, llvm::PointerType::getUnqual(Builder.getInt8PtrTy()));
  globalScope->setNamedVal(
      funcName,
      new CompileVal(castedResult, func->getCompileType(), /*isBoxed=*/false),
      0);

  return result;
}

CompileVal *AstWalker::codeGen_FuncDef(Json::Value &jsonNode) {
  if (scopeHelper->getCurrScopeType() != ScopeNode::ScopeType::TOP_SCOPE) {
    GEN_FAIL("Nested functions are not yet implemented");
  }

  pushScope(ScopeNode::ScopeType::FUNC_SCOPE);

  Json::Value funcDefNode = jsonNodeGet(jsonNode, "PlainFunc");
  CompileVal *compileFunc = makeFuncProto(funcDefNode);
  llvm::Function *func =
      static_cast<llvm::Function *>(compileFunc->getRawValue());
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(currContext, "varDecls", func);
  llvm::BasicBlock *funcBody =
      llvm::BasicBlock::Create(currContext, "funcBody");
  std::string funcName = funcDefNode["name"].asString();
  scopeHelper->setBlockOnCurrScope(entry);
  Builder.SetInsertPoint(entry);
  createGlobalFunctionConst(funcName, compileFunc);

  if (funcName == "main") {
    codeGenHelper->createNativeCall("initialize_core", {});
  }

  int arg_index = 0;
  for (auto argI = func->arg_begin(); arg_index < ((int)func->arg_size());
       argI++, arg_index++) {
    newVarInScope(
        argI->getName(),
        new CompileVal(
            &(*argI),
            makeCompileType(funcDefNode["header"]["FuncProto"]["args"]
                                       [arg_index]["TypedArg"]["type"])));
  }

  startBlock(funcBody);

  Builder.SetInsertPoint(entry);
  Builder.CreateBr(funcBody);
  Builder.SetInsertPoint(funcBody);
  CompileType const *returnType =
      compileFunc->getCompileType()->getFunctionReturnType();
  CompileVal *returnVal = codeGen_initial(funcDefNode["simple_stmt"]);

  llvm::BasicBlock *originalBlock = Builder.GetInsertBlock();
  llvm::BasicBlock &func_block = originalBlock->getParent()->getEntryBlock();

  uint64_t numVarsInFunc = scopeHelper->getNumNamedVarsSinceFunc();
  Builder.SetInsertPoint(&func_block);
  codeGenHelper->createNativeCall(
      "gc_push_func_scope",
      {codeGenHelper->getConstInt64(numVarsInFunc, false)});
  Builder.SetInsertPoint(originalBlock);

  if (CompileType::isVoidType(returnType)) {
    createReturn(nullptr);
  } else {
    // XXX Assumes return type specified in function proto is complete.
    GEN_ASSERT(CompileType::lhsTypeCanReplaceRhsType(
                   returnType, returnVal->getCompileType()),
               "Return type for function " + funcName + " is incorrect.\n" +
                   "Expected: " + returnType->getLongTypeName() +
                   "\nBut got: " +
                   returnVal->getCompileType()->getLongTypeName());
    createReturn(returnVal);
  }

  popScope();

  return compileFunc;
}

CompileVal *AstWalker::createReturn(CompileVal *val) {
  codeGenHelper->createNativeCall("gc_pop_scope", {});
  // All return values must be of boxed type.
  if (val != nullptr && !val->getIsBoxed()) {
    val = codeGenHelper->boxValue(val);
  }
  if (val == nullptr || CompileType::isVoidType(val->getCompileType())) {
    return new CompileVal(Builder.CreateRetVoid(),
                          CompileType::CommonType::VOID);
  } else {
    return new CompileVal(Builder.CreateRet(val->getRawValue()),
                          val->getCompileType());
  }
}

CompileVal *AstWalker::codeGen_ListOp(Json::Value &jsonNode) {
  int numListItems = jsonNode.size();
  CompileVal *list = codeGenHelper->createLiteral(
      CompileType::CommonType::LIST,
      codeGenHelper->getConstInt64(numListItems, false));
  for (unsigned i = 0; i < jsonNode.size(); i++) {
    CompileVal *list_el = codeGen_initial(jsonNode[i]);
    // Initialize list type.
    if (i == 0) {
      // Make generic list parameter concrete here.
      list->setCompileType(CompileType::implementGeneric(
          CodeGenUtil::LIST_GENERIC_PARAM, list_el->getCompileType(),
          list->getCompileType()));
    }
    createClassMethodCall(
        "set", list,
        {codeGenHelper->createLiteral(CompileType::CommonType::INT,
                                      codeGenHelper->getConstInt64(i, false)),
         list_el});
  }

  return list;
}

CompileVal *AstWalker::codeGen_BracExpr(Json::Value &jsonNode) {
  CompileVal *result = nullptr;
  for (auto &currNode : jsonNode) {
    result = codeGen_initial(currNode);
  }

  return result;
}

CompileVal *AstWalker::codeGen_UnOp(Json::Value &jsonNode) {
  const std::string op = jsonNode["op"].asString();

  std::string opFuncPrefix = "";

  if (op == "-") {
    opFuncPrefix = "neg";
  } else {
    GEN_FAIL("Unimplemented unary operator type " + op);
  }
  CompileVal *atomExpr = codeGen_AtomOp(jsonNode["atom"]);
  return createClassMethodCall(opFuncPrefix, atomExpr);
}

CompileVal *AstWalker::codeGen_ListGen(Json::Value &jsonNode) {
  // TODO This could make use of a desugaring pass that just uses
  // the ForOp node.
  llvm::BasicBlock *loopTop = makeBasicBlock("loopTop");
  llvm::BasicBlock *loopBody =
      llvm::BasicBlock::Create(currContext, "loopBody");
  llvm::BasicBlock *loopBottom =
      llvm::BasicBlock::Create(currContext, "loopBottom");

  pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);

  CompileVal *itt = codeGen_AtomOp(jsonNode["itt"]);
  std::string loopVarName = jsonNode["loop_var"].asString();
  CompileVal *list = codeGenHelper->createLiteral(
      CompileType::CommonType::LIST, codeGenHelper->getConstInt64(0, false));

  startForLoop(itt, loopVarName, loopTop, loopBody, loopBottom);
  CompileVal *expr = codeGen_ExprOp(jsonNode["element_expr"]);
  // Make list generic type concrete.
  list->setCompileType(CompileType::implementGeneric(
      CodeGenUtil::LIST_GENERIC_PARAM, expr->getCompileType(),
      list->getCompileType()));
  // Body of loop. Add result of expr to list.
  createClassMethodCall("append", list, {expr});

  endForLoop(itt, loopVarName, loopTop, loopBottom);

  popScope();

  return list;
}

CompileVal *AstWalker::codeGen_initial(Json::Value &jsonNode) {
  TRY_NODE(jsonNode, StmtsOp);
  TRY_NODE(jsonNode, ExprOp);
  TRY_NODE(jsonNode, VarDef);
  TRY_NODE(jsonNode, BinOp);
  TRY_NODE(jsonNode, AtomOp);
  TRY_NODE(jsonNode, ForOp);
  TRY_NODE(jsonNode, WhileOp);
  TRY_NODE(jsonNode, IfOp);
  TRY_NODE(jsonNode, FuncDef);
  TRY_NODE(jsonNode, ListOp);
  TRY_NODE(jsonNode, BracExpr);
  TRY_NODE(jsonNode, UnOp);
  TRY_NODE(jsonNode, ListGen);
  //  TRY_NODE(jsonNode, ClassDef);

  // If none of the TRY_NODE blocks returned anything, then we have an
  // unimplemented ast node.
  cout << jsonNode << endl;
  GEN_FAIL("Unimplemented node type in code generator");

  return nullptr;
}

void AstWalker::codeGen_top(std::string jsonString) {
  Json::Value jsonNode = generateFromJson(jsonString);
  codeGen_initial(jsonNode);
}

Json::Value AstWalker::generateFromJson(std::string jsonString) {
  Json::Reader reader;
  Json::Value json_root;
  reader.parse(jsonString, json_root);
  return json_root;
}

CompileVal *AstWalker::createClassMethodCall(
    const std::string &methodName, CompileVal *thisObj,
    const std::vector<CompileVal *> &args) {
  CompileVal *func = codeGenHelper->createVtableAccess(thisObj, methodName);
  std::vector<CompileVal *> newArgs = {thisObj};
  for (auto el : args) {
    newArgs.push_back(el);
  }
  return createLangCall(func, newArgs);
}

llvm::Type *AstWalker::rawTypeFromCompileType(CompileType const *compileType) {
  if (compileType->isVoidType()) {
    return llvm::Type::getVoidTy(currContext);
  } else {
    return Builder.getInt8PtrTy();
  }
}

llvm::Module *AstWalker::getModule() { return currModule; }

llvm::LLVMContext *AstWalker::getContext() { return &currContext; }

}  // namespace codegen
