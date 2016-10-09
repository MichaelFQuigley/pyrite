#include <iostream>
#include <cstdlib>
#include "ast_walker.h"

using namespace std;

AstWalker::AstWalker(std::string filename, std::string stdlib_filename) : Builder(currContext)
{
    currModule    = llvm::make_unique<llvm::Module>(filename, currContext);
    scopeHelper   = new ScopeHelper();
    codeGenHelper = new CodeGenUtil(currModule.get(), &currContext);

    if( !codeGenHelper->load_stdlib(stdlib_filename) )
    {
        return;
    }

    //TODO automate this process...
    CompileVal* printlnVal = new CompileVal(tryGetFunction("println"),
                                  new CompileType(CompileType::CommonType::FUNCTION));
    printlnVal->setArgumentsList(new std::vector<CompileType*>({
                                    new CompileType(CompileType::CommonType::STRING),
                                    new CompileType(CompileType::CommonType::VOID)}));
    addFuncPtr("println", printlnVal);

    CompileVal* String_IntVal = new CompileVal(tryGetFunction("String_Int"),
                                    new CompileType(CompileType::CommonType::FUNCTION));
    String_IntVal->setArgumentsList(new std::vector<CompileType*>({
                                        new CompileType(CompileType::CommonType::INT),
                                        new CompileType(CompileType::CommonType::STRING)}));
    addFuncPtr("String_Int", String_IntVal);
}

void AstWalker::addFuncPtr(std::string funcName, CompileVal* func)
{
    if( scopeHelper->getCurrScopeType() == ScopeNode::ScopeType::TOP_SCOPE )
    {
        createGlobalFunctionConst(funcName, func);
    }
    else
    {
        GEN_FAIL("Nested functions not implemented yet.");
    }
}

bool AstWalker::jsonNode_has(Json::Value jsonNode, std::string name, Json::Value* out_node)
{
    if( jsonNode[name] != Json::nullValue )
    {
        *out_node = jsonNode[name];
        return true;
    }

    return false;
}

CompileVal* AstWalker::codeGen_StmtsOp(Json::Value jsonNode)
{
    CompileVal* result = nullptr;
    for(const auto& stmt_node : jsonNode)
    {
        result = codeGen_initial(stmt_node);
    }

    return result;
}

void AstWalker::pushScope(ScopeNode::ScopeType scopeType, bool funcScopeRetVoid)
{
    scopeHelper->pushScope(scopeType, funcScopeRetVoid);
}

void AstWalker::popScope()
{
    scopeHelper->popScope();
}

CompileType* AstWalker::makeCompileType(Json::Value jsonNode)
{
    CompileType* result = nullptr;
    if( jsonNode["simple"] != Json::nullValue )
    {
        result = new CompileType(jsonNode["simple"].asString());
    }
    else if( jsonNode["list_type"] != Json::nullValue )
    {
        result =  new CompileType(CompileType::CommonType::LIST); 
        result->insertArgumentsList(makeCompileType(jsonNode["list_type"]));
    }
    else
    {
        cout << jsonNode << endl;
        GEN_FAIL("Unimplemented compile type for makeCompileType");
    }

    return result;
}

CompileVal* AstWalker::makeFuncProto(Json::Value jsonNode)
{
    std::vector<llvm::Type*> argsV;
    Json::Value header_node                = jsonNode["header"]["FuncProto"];
    Json::Value args_node                  = header_node["args"];
    std::vector<CompileType*>* compileArgs = new std::vector<CompileType*>();

    for( const Json::Value& val : args_node )
    {
        argsV.push_back(Builder.getInt8PtrTy());
        compileArgs->push_back(makeCompileType(val["TypedArg"]["type"]));
    }
    CompileType* compileRetType = makeCompileType(header_node["ret_type"]);
    compileArgs->push_back(compileRetType); 
    std::string funcName = jsonNode["name"].asString();
    llvm::Type* ret_type = compileRetType->getTypeName()
                           == CompileType::getCommonTypeName(CompileType::CommonType::VOID) ?
                               llvm::Type::getVoidTy(currContext)
                               : Builder.getInt8PtrTy();
    llvm::FunctionType* funcProto = llvm::FunctionType::get(ret_type, 
                                                                argsV, 
                                                                false);    
    llvm::Function* func          = llvm::Function::Create(funcProto, 
                                                            llvm::Function::ExternalLinkage, 
                                                            funcName,
                                                            currModule.get());
    int arg_index = 0;
    for(auto argI = func->arg_begin(); 
            arg_index < ((int)func->arg_size()); 
            argI++, arg_index++)
    {
        argI->setName(header_node["args"][arg_index]["TypedArg"]["name"].asString());
    }

    CompileVal* result = new CompileVal(func, CompileType::CommonType::FUNCTION); 
    result->setArgumentsList(compileArgs);

    return result;
}

CompileVal* AstWalker::codeGen_ExprOp(Json::Value jsonNode){ return codeGen_initial(jsonNode); }

CompileVal* AstWalker::codeGen_VarDef(Json::Value jsonNode){
    Json::Value tempNode;

    if( jsonNode_has(jsonNode, "definition", &tempNode) )
    {
        std::string varName        = tempNode["var"]["TypedArg"]["name"].asString();
        CompileType* annotatedType = makeCompileType(tempNode["var"]["TypedArg"]["type"]);
        CompileVal* rhs_expr       = codeGen_initial(tempNode["expr"]); 
        GEN_ASSERT(annotatedType->isCompatibleWithType(rhs_expr->getCompileType()),
                   "Annotated type is not compatible with type on right hand side!");
        rhs_expr->setCompileType(annotatedType);
        GEN_ASSERT(rhs_expr != nullptr, "Invalid assignment to rhs of variable definition.");
        newVarInScope(varName, rhs_expr); 
    }
    else if( jsonNode_has(jsonNode, "definition_infer", &tempNode) )
    {
        std::string varName   = tempNode["name"].asString();
        CompileVal* rhs_expr  = codeGen_initial(tempNode["expr"]); 
        GEN_ASSERT(rhs_expr != nullptr, "Invalid assignment to rhs of variable definition.");
        newVarInScope(varName, rhs_expr); 
    }
    else if( jsonNode_has(jsonNode, "declaration", &tempNode) )
    {
        std::string varName      = tempNode["TypedArg"]["name"].asString();
        CompileType* compileType = makeCompileType(tempNode["TypedArg"]["type"]);
        newVarInScope(varName, new CompileVal(nullptr, compileType), false);
    }
    else
    {
        GEN_FAIL("Invalid variable definition.");
    }

    return nullptr; 
}

CompileVal* AstWalker::newVarInScope(std::string varName, CompileVal* value, bool is_definition)
{
    llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
    llvm::BasicBlock& func_block    = originalBlock->getParent()->getEntryBlock();
    llvm::Value* allocaRes;
    
    Builder.SetInsertPoint(&func_block);
    allocaRes = Builder.CreateAlloca(Builder.getInt8PtrTy());
    Builder.SetInsertPoint(originalBlock);

    if( is_definition )
    {
        Builder.CreateStore(value->getRawValue(), allocaRes);
    }

    CompileVal* result = new CompileVal(allocaRes, value->getCompileType());
    scopeHelper->setNamedVal(varName, result, true);
   
    return result;
}

void AstWalker::handleAssignLhs(Json::Value assignLhs, CompileVal* rhs)
{
        Json::Value lhs_val  = assignLhs["Id"];
        std::string id_name  = lhs_val["name"].asString();
        Json::Value trailers = lhs_val["trailers"];
        CompileVal* result   = nullptr;
        CompileVal* var_val  = scopeHelper->getNamedVal(id_name, true);

        GEN_ASSERT(var_val != nullptr, "Variable " + id_name + " is undefined.");

        result = new CompileVal(Builder.CreateLoad(var_val->getRawValue()),
                                                   var_val->getCompileType());

        if( trailers.size() > 0 )
        {
            for( int i = 0; i < trailers.size(); i++ )
            {
                if( trailers[i]["FCall"] != Json::nullValue )
                {
                    GEN_ASSERT(i < trailers.size() - 1, "Function call is not assignable!");

                    std::vector<CompileVal*>* fcallArgs = new std::vector<CompileVal*>();
                    for(const Json::Value& val : trailers[i]["FCall"]["args"])
                    {
                        fcallArgs->push_back(codeGen_initial(val));
                    }
                    
                    result = createLangCall(result, fcallArgs);               
                }
                else if( trailers[i]["Index"] != Json::nullValue )
                {
                    // TODO typecheking on indexVal
                    CompileVal* indexVal    = codeGen_initial(trailers[i]["Index"]);

                    if( i < trailers.size() - 1 )
                    {
                        std::string lhsTypename = result->getCompileType()->getTypeName();
                        //The type of return value from the call to 'get_*' will always be the 
                        //first argument in the compile type's argument list.
                        CompileType* returnedType = (*(result->getCompileType()->getArgumentsList()))[0];
                        result = new CompileVal(createNativeCall("get_" + lhsTypename,
                                                                 {result->getRawValue(),
                                                                 indexVal->getRawValue()}), 
                                                returnedType);
                    }
                    else 
                    {
                        //XXX For now, assumes result type is complete.
                        CompileType* resultGenericType = (*(result->getCompileType()
                                                            ->getArgumentsList()))[0];
                        GEN_ASSERT(resultGenericType->isCompatibleWithType(rhs->getCompileType()),
                                   "Type on rhs is not compatible with lhs in assignment of "
                                   + var_val->getCompileType()->getTypeName() + " "
                                   + id_name + ".");
                        createNativeCall("set_" + result->getCompileType()->getTypeName(), 
                                         {result->getRawValue(),
                                          indexVal->getRawValue(),
                                          rhs->getRawValue()});
                    }
                }
            }
        }
        else
        {
            std::vector<llvm::Value*> argsV;
            uint64_t varIndex         = scopeHelper->getNamedValInd(id_name);

            GEN_ASSERT(result->getCompileType()->isCompatibleWithType(rhs->getCompileType()),
                       "Type on rhs is not compatible with lhs in assignment of "
                       + var_val->getCompileType()->getTypeName() + " "
                       + id_name + ".");
            Builder.CreateStore(rhs->getRawValue(), var_val->getRawValue());
            argsV.push_back(Builder.CreateLoad(var_val->getRawValue()));
            argsV.push_back(codeGenHelper->getConstInt64(varIndex, false));
            createNativeCall("gc_set_named_var_in_scope", argsV);
        }
}

CompileVal* AstWalker::codeGen_BinOp(Json::Value jsonNode){
    std::string op = jsonNode["op"].asString();
    std::string opFuncPrefix = "";
    bool isCompare = false;

    if( op == "=" ) 
    {
        Json::Value lhs_node;
        if( jsonNode_has(jsonNode["lhs"], "AtomOp", &lhs_node) )
        {
            CompileVal* rhs_val = codeGen_initial(jsonNode["rhs"]);

            GEN_ASSERT(rhs_val != nullptr, "Invalid expression on rhs of assignment.");

            handleAssignLhs(lhs_node, rhs_val); 

            return rhs_val;
        }
        else
        {
            GEN_FAIL("Invalid lhs of assignment.");
        }
    }
    else if( op == "+" )  { opFuncPrefix = "add";}
    else if( op == "-" )  { opFuncPrefix = "sub";}
    else if( op == "*" )  { opFuncPrefix = "mul";}
    else if( op == "/" )  { opFuncPrefix = "div";}
    else if( op == "%" )  { opFuncPrefix = "mod";}
    else if( op == "&" )  { opFuncPrefix = "and";}
    else if( op == "|" )  { opFuncPrefix = "or";}
    else if( op == "^" )  { opFuncPrefix = "xor";}
    else if( op == "<" )  { opFuncPrefix = "cmplt";}
    else if( op == "<=" ) { opFuncPrefix = "cmple"; isCompare = true;}
    else if( op == "==" ) { opFuncPrefix = "cmpeq"; isCompare = true;}
    else if( op == ">" )  { opFuncPrefix = "cmpgt"; isCompare = true;}
    else if( op == ">=" ) { opFuncPrefix = "cmpge"; isCompare = true;}
    else if( op == "!=" ) { opFuncPrefix = "cmpne"; isCompare = true;}
    else { 
        GEN_FAIL("Unimplemented operator type!");
    }

    CompileVal* lhs          = codeGen_initial(jsonNode["lhs"]);
    std::string op_func_name = opFuncPrefix + "_" + lhs->getCompileType()->getTypeName();
    CompileVal* rhs          = codeGen_initial(jsonNode["rhs"]); 

    llvm::Value* result_val = createNativeCall(op_func_name,
                                               {lhs->getRawValue(),
                                               rhs->getRawValue()});

    CompileType* rhsType = rhs->getCompileType();
    CompileType* lhsType = lhs->getCompileType();
    // Complete type for lhs and rhs.
    CompileType* argsCompleteType;
    if( lhsType->isCompatibleWithType(rhsType) )
    {
        argsCompleteType = lhsType;
    }
    else if( rhsType->isCompatibleWithType(lhsType) )
    {
        argsCompleteType = rhsType;
    }
    else
    {
        GEN_FAIL(lhsType->getTypeName()
                + " is not compatible with "
                + rhsType->getTypeName()
                + " in binary expression.");
    }

    return new CompileVal(result_val, isCompare ? 
                                          new CompileType(CompileType::CommonType::BOOL)
                                        : lhs->getCompileType());
}

CompileVal* AstWalker::codeGen_AtomOp(Json::Value jsonNode) { 
    Json::Value val_node = Json::nullValue;

    if( jsonNode_has(jsonNode, "Lit", &val_node) ) 
    {
        std::string lit_str = val_node.asString();
        std::string lit_val = lit_str.substr(1);
        char lit_type       = lit_str[0];

        double double_val;
        long int_val;
        // Literal types have a prefix specifying their kind.
        switch( lit_type )
        {
            // Float
            case 'f':
                double_val = stod(lit_val);
                return createConstObject(CompileType::CommonType::FLOAT,
                            llvm::ConstantFP::get(currContext, llvm::APFloat(double_val)));
            // Int
            case 'i':
                int_val = stol(lit_val);
                return createConstObject(CompileType::CommonType::INT, Builder.getInt64(int_val));
            // String
            case 's':
                 return createConstObject(CompileType::CommonType::STRING, 
                                    codeGenHelper->generateString(lit_val));
            // Bool
            case 'b':
                return createConstObject(CompileType::CommonType::BOOL,
                                         Builder.getInt1(lit_val == "true"));
            default:
                GEN_FAIL("Unimplemented literal type");
        }
    }
    else if( jsonNode_has(jsonNode, "Id", &val_node) )
    {
        std::string id_name  = val_node["name"].asString();
        Json::Value trailers = val_node["trailers"];
        CompileVal* result   = nullptr;

        CompileVal* var_val = scopeHelper->getNamedVal(id_name, true);

        GEN_ASSERT(var_val != nullptr, "Variable " + id_name + " is undefined.");

        result = new CompileVal(Builder.CreateLoad(var_val->getRawValue()), var_val->getCompileType());

        for(const auto& trailer : trailers)
        {
            if( trailer["FCall"] != Json::nullValue )
            {
                std::vector<CompileVal*>* fcallArgs = new std::vector<CompileVal*>();
                for(const Json::Value& val : trailer["FCall"]["args"])
                {
                    fcallArgs->push_back(codeGen_initial(val));
                }
                
                result = createLangCall(result, fcallArgs);               
            }
            else if( trailer["Index"] != Json::nullValue )
            {
                std::vector<llvm::Value*> argsV;
                CompileVal* indexVal    = codeGen_initial(trailer["Index"]);
                argsV.push_back(result->getRawValue());
                argsV.push_back(indexVal->getRawValue());
                std::string lhsTypename = result->getCompileType()->getTypeName();
                //The type of return value from the call to 'get_*' will always be the 
                //first argument in the compile type's argument list.
                CompileType* returnedType = (*(result->getCompileType()->getArgumentsList()))[0];
                result = new CompileVal(createNativeCall("get_" + lhsTypename, argsV), returnedType);
            }
        }

        return result;
    }
    else if( jsonNode_has(jsonNode, "ParenExpr", &val_node) )
    {
        return codeGen_ExprOp(val_node);
    }
    else if( jsonNode_has(jsonNode, "RangeOp", &val_node) )
    {
        CompileVal* start = codeGen_AtomOp(val_node["start"]);
        CompileVal* step  = createConstObject(CompileType::CommonType::INT, 
                                                llvm::ConstantInt::get(currContext, 
                                                llvm::APInt(64, 1, true)));
        CompileVal* end   = codeGen_AtomOp(val_node["end"]);
        
        GEN_ASSERT(start->typesAreEqual(end),
                "Start and end type for range must be the same.");
        //XXX For now, RangeOp is assumed to iterate over Ints.
        CompileType* iteratedType = start->getCompileType();
        CompileType* rangeType = new CompileType(
                iteratedType->getTypeName() + "Range");
        rangeType->insertArgumentsList(iteratedType);

        llvm::Value* init_RangeResult = createNativeCall(
                "init_" + rangeType->getTypeName(), 
                { start->getRawValue(),
                  step->getRawValue(),
                  end->getRawValue() });
        return new CompileVal(init_RangeResult, rangeType);
    }
    else
    {
        GEN_FAIL("Unimplemented atom value.");
    }
    return nullptr;
}

llvm::BasicBlock* AstWalker::makeBasicBlock(std::string name)
{
    return llvm::BasicBlock::Create(currContext, 
                                    name, 
                                    Builder.GetInsertBlock()->getParent());
}

llvm::Function* AstWalker::tryGetFunction(std::string func_name,
        bool raise_fail_exception, 
        std::string error_msg)
{
    std::string full_func_name = func_name;
    auto func                  = currModule->getFunction(full_func_name);

    if( raise_fail_exception )
    {
        GEN_ASSERT(func != nullptr, full_func_name + " " + error_msg);
    }

    return func;   
}

CompileVal* AstWalker::createLangCall(CompileVal* func,
        std::vector<CompileVal*>* argsV)
{
    GEN_ASSERT(func->getCompileType()->getTypeName() == "Function", 
            "Error: Trying to call something that is not a function!");
    std::vector<llvm::Value*> nativeArgs;
    std::vector<CompileType*>* funcProtoArgs = (func->getCompileType()->getArgumentsList());
    CompileType* retType = func->getCompileType()->getFunctionReturnType();

    GEN_ASSERT(argsV->size() == funcProtoArgs->size() - 1, 
            string("Number of arguments in function call must match.\n")
            + "Expected " + to_string(funcProtoArgs->size() - 1)  
            + " arguments, but received " + to_string(argsV->size()) + ".");

    for(int i = 0; i < argsV->size(); i++)
    {
        GEN_ASSERT((*funcProtoArgs)[i]->isCompatibleWithType((*argsV)[i]->getCompileType()),
                "Types used in call to function do not match function prototype."); 
        nativeArgs.push_back((*argsV)[i]->getRawValue());
    }

    llvm::Value* retVal = Builder.CreateCall(func->getRawValue(), nativeArgs);

    return new CompileVal(retVal, retType);
}

llvm::Value* AstWalker::createNativeCall(std::string func_name, 
        std::vector<llvm::Value*> argsV)
{
    auto func            = tryGetFunction(func_name);
    llvm::Value* ret_val = Builder.CreateCall(func, argsV);

    return ret_val;
}

void AstWalker::createBoolCondBr(llvm::Value* Bool, 
        llvm::BasicBlock* trueBlock,
        llvm::BasicBlock* falseBlock)
{
    llvm::Value* raw_bool = createNativeCall("rawVal_Bool", {Bool});

    Builder.CreateCondBr(raw_bool, trueBlock, falseBlock);
}

CompileVal* AstWalker::codeGen_ForOp(Json::Value jsonNode){ 
    llvm::BasicBlock* loopTop    = makeBasicBlock("loopTop");
    llvm::BasicBlock* loopBody   = llvm::BasicBlock::Create(currContext, "loopBody");
    llvm::BasicBlock* loopBottom = llvm::BasicBlock::Create(currContext, "loopBottom");

    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);

    CompileVal* itt                = codeGen_AtomOp(jsonNode["itt"]);
    CompileType* iteratorType       = itt->getCompileType();
    std::string iteratorTypeName    = iteratorType->getTypeName();
    std::string itt_hasNextFuncName = "hasNext_" + iteratorTypeName;
    std::string itt_nextFuncName    = "next_" + iteratorTypeName;
    std::string itt_beginFuncName   = "begin_" + iteratorTypeName;
    std::string loop_var_name       = jsonNode["loop_var"].asString();

    llvm::Value* loop_var = createNativeCall(itt_beginFuncName, {itt->getRawValue()});
    CompileType* loopVarType = (*(iteratorType->getArgumentsList()))[0];
    newVarInScope(loop_var_name, new CompileVal(loop_var, loopVarType));

    Builder.CreateBr(loopTop);
    Builder.SetInsertPoint(loopTop);

    llvm::Value* hasNext          = createNativeCall(itt_hasNextFuncName, {itt->getRawValue()});

    createBoolCondBr(hasNext, loopBody, loopBottom);
    //loop body
    startBlock(loopBody);
    createNativeCall("gc_push_loop_scope", {});
    codeGen_initial(jsonNode["body"]);
    Builder.CreateStore(createNativeCall(itt_nextFuncName, {itt->getRawValue()}), 
                        scopeHelper->getNamedVal(loop_var_name, true)->getRawValue());

    createNativeCall("gc_pop_scope", {});
    Builder.CreateBr(loopTop);
    startBlock(loopBottom);

    popScope();

    return nullptr;
}

CompileVal* AstWalker::codeGen_WhileOp(Json::Value jsonNode)
{ 
    llvm::BasicBlock* loopTop    = llvm::BasicBlock::Create(currContext, "whileTop");
    llvm::BasicBlock* loopBody   = llvm::BasicBlock::Create(currContext, "whileBody");
    llvm::BasicBlock* loopBottom = llvm::BasicBlock::Create(currContext, "whileBottom");

    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
    
    Builder.CreateBr(loopTop);
    startBlock(loopTop);

    CompileVal* header = codeGen_initial(jsonNode["header"]);
    createBoolCondBr(header->getRawValue(), loopBody, loopBottom);

    startBlock(loopBody);
    codeGen_initial(jsonNode["body"]);

    Builder.CreateBr(loopTop);
    startBlock(loopBottom);

    popScope();

    return nullptr;
}

void AstWalker::startBlock(llvm::BasicBlock* block)
{
    llvm::Function *currFunction = Builder.GetInsertBlock()->getParent();
    currFunction->getBasicBlockList().push_back(block);
    Builder.SetInsertPoint(block);
}

CompileVal* AstWalker::codeGen_IfOp(Json::Value jsonNode)
{
    CompileVal* test = codeGen_initial(jsonNode["test"]);
    
    llvm::BasicBlock* ifTrue = makeBasicBlock("ifTrue");
    llvm::BasicBlock* endIf;

    CompileVal* result = nullptr;
    //check to see if there is just an if block without an else
    if( jsonNode["bodies"].size() == 1 )
    {
        pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
        endIf = llvm::BasicBlock::Create(currContext, "endif");
        createBoolCondBr(test->getRawValue(), ifTrue, endIf);
        Builder.SetInsertPoint(ifTrue);
        codeGen_initial(jsonNode["bodies"][0]);
        Builder.CreateBr(endIf);
        startBlock(endIf);
        popScope();
    }
    //else block (size == 2)
    else
    {
        llvm::BasicBlock* ifFalse = llvm::BasicBlock::Create(currContext, "ifFalse");
        endIf = llvm::BasicBlock::Create(currContext, "endif");
        createBoolCondBr(test->getRawValue(), ifTrue, ifFalse);
        //if true codegen
        pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
        Builder.SetInsertPoint(ifTrue);
        CompileVal* ifTrueLastStmt = codeGen_initial(jsonNode["bodies"][0]);
        popScope();
        //if false codegen
        pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
        Builder.CreateBr(endIf);
        ifTrue = Builder.GetInsertBlock();
        startBlock(ifFalse);
        CompileVal* ifFalseLastStmt = codeGen_initial(jsonNode["bodies"][1]);
        Builder.CreateBr(endIf);
        ifFalse = Builder.GetInsertBlock();
        startBlock(endIf);
        popScope();
        //phi node
        if( ifTrueLastStmt != nullptr && ifFalseLastStmt != nullptr 
            && !( CompileType::isVoidType(ifTrueLastStmt->getCompileType()) )
            && !( CompileType::isVoidType(ifFalseLastStmt->getCompileType())) )
        {
            llvm::PHINode * phi = Builder.CreatePHI(ifTrueLastStmt->getRawValue()->getType(), 2, "ifPhi");
            phi->addIncoming(ifTrueLastStmt->getRawValue(), ifTrue);
            phi->addIncoming(ifFalseLastStmt->getRawValue(), ifFalse);
            result = new CompileVal(phi, ifTrueLastStmt->getCompileType());
        }
        else
        {
            result = nullptr;
        }
    }

    return result;
}

llvm::Value* AstWalker::createGlobalFunctionConst(std::string funcName, CompileVal* func)
{
    llvm::Function* initValue    = dynamic_cast<llvm::Function*>(func->getRawValue());
    ScopeNode* globalScope       = scopeHelper->getNearestScopeOfType(ScopeNode::ScopeType::TOP_SCOPE);
    llvm::GlobalVariable* result = new llvm::GlobalVariable(
                                            *currModule.get(), 
                                            initValue->getType(), 
                                            false,
                                            llvm::GlobalValue::PrivateLinkage,
                                            initValue,
                                            funcName);
    globalScope->setNamedVal(funcName, new CompileVal(result, func->getCompileType()), 0);    

    return result;
}

CompileVal* AstWalker::codeGen_FuncDef(Json::Value jsonNode)
{
    if( scopeHelper->getCurrScopeType() == ScopeNode::ScopeType::TOP_SCOPE )
    {
        pushScope(ScopeNode::ScopeType::FUNC_SCOPE);

        CompileVal* compileFunc    = makeFuncProto(jsonNode);
        llvm::Function* func       = static_cast<llvm::Function*>(compileFunc->getRawValue());
        llvm::BasicBlock* entry    = llvm::BasicBlock::Create(currContext, "varDecls", func);
        llvm::BasicBlock* funcBody = llvm::BasicBlock::Create(currContext, "funcBody");
        std::string funcName       = jsonNode["name"].asString();
        scopeHelper->setBlockOnCurrScope(entry);
        Builder.SetInsertPoint(entry);
        createGlobalFunctionConst(funcName, compileFunc);

        if( funcName == "main" )
        {
            createNativeCall("initialize_core", {});
        }

        int arg_index = 0;
        for(auto argI = func->arg_begin(); arg_index < ((int)func->arg_size()); argI++, arg_index++)
        {
            newVarInScope(argI->getName(), 
                    new CompileVal(argI, 
                        makeCompileType(jsonNode["header"]
                                                 ["FuncProto"]
                                                 ["args"]
                                                 [arg_index]
                                                 ["TypedArg"]
                                                 ["type"])));
        }

        startBlock(funcBody);
        
        Builder.SetInsertPoint(entry);
        Builder.CreateBr(funcBody);
        Builder.SetInsertPoint(funcBody);
        CompileType* returnType = compileFunc->getCompileType()->getFunctionReturnType();
        CompileVal* returnVal   = codeGen_initial(jsonNode["simple_stmt"]);

        llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
        llvm::BasicBlock& func_block    = originalBlock->getParent()->getEntryBlock();

        uint64_t numVarsInFunc = scopeHelper->getNumNamedVarsSinceFunc();
        Builder.SetInsertPoint(&func_block);
        createNativeCall("gc_push_func_scope", {codeGenHelper->getConstInt64(numVarsInFunc, false)}); 
        Builder.SetInsertPoint(originalBlock);

        if( CompileType::isVoidType(returnType) )
        {
            createReturn(nullptr);
        }
        else
        {
            //XXX Assumes return type specified in function proto is complete.
            GEN_ASSERT(returnType->isCompatibleWithType(returnVal->getCompileType()),
                       "Return type for function " + funcName + " is incorrect.");
            createReturn(returnVal);
        }

        popScope();

        return compileFunc; 
    }
    else
    {
        GEN_FAIL("Nested functions are not yet implemented");
    }
}

CompileVal* AstWalker::createReturn(CompileVal* val)
{
    createNativeCall("gc_pop_scope", {});
    if( val == nullptr || CompileType::isVoidType(val->getCompileType()) )
    {
        return new CompileVal(Builder.CreateRetVoid(), CompileType::CommonType::VOID);
    }
    else
    {
        return new CompileVal(Builder.CreateRet(val->getRawValue()), val->getCompileType()); 
    }
}

CompileVal* AstWalker::codeGen_ListOp(Json::Value jsonNode)
{

    std::vector<llvm::Value*> argsV;
    int numListItems = jsonNode.size();
    CompileVal* list = createConstObject(CompileType::CommonType::LIST,
                                         codeGenHelper->getConstInt64(numListItems,
                                                                      false));
    CompileType* listType = nullptr;
    for( unsigned i = 0; i < jsonNode.size(); i++ )
    {
        argsV.clear();
        CompileVal* list_el = codeGen_initial(jsonNode[i]);
        // Initialize list type.
        if( i == 0 )
        {
            listType = list_el->getCompileType();
            list->insertArgumentType(listType);
        }
        GEN_ASSERT(listType->isEqualToType(list_el->getCompileType()),
                "Types in list are inconsistent!");
        argsV.push_back(list->getRawValue());
        // index 
        argsV.push_back(createConstObject(CompileType::CommonType::INT,
                                          codeGenHelper->getConstInt64(i,
                                                                       false))->getRawValue());
        // value 
        argsV.push_back(list_el->getRawValue());
        createNativeCall("set_List", argsV);
    }

    return list;   
}

CompileVal* AstWalker::codeGen_BracExpr(Json::Value jsonNode)
{
    CompileVal* result = nullptr;
    for(const auto& currNode : jsonNode)
    {
        result = codeGen_initial(currNode);
    }

    return result;
}

CompileVal* AstWalker::codeGen_UnOp(Json::Value jsonNode)
{
    std::string op = jsonNode["op"].asString();

    std::string opFuncPrefix = "";

    if( op == "-" ) { opFuncPrefix = "neg"; }
    else 
    {
        GEN_FAIL("Unimplemented unary operator type " + op);
    }

    CompileVal* atomExpr    = codeGen_AtomOp(jsonNode["atom"]); 
    std::string opFuncName  = opFuncPrefix + "_" + atomExpr->getCompileType()->getTypeName();
    llvm::Value* result_val = createNativeCall(opFuncName,
                                               {atomExpr->getRawValue()});

    return new CompileVal(result_val, atomExpr->getCompileType());
}

CompileVal* AstWalker::codeGen_ListGen(Json::Value jsonNode)
{
    CompileVal* itt                     = codeGen_AtomOp(jsonNode["itt"]);
    std::vector<CompileType*>* typeArgs = itt->getCompileType()->getArgumentsList();
    // TODO Improve checking for iterable values.
    GEN_ASSERT(typeArgs->size() >= 1, "Not a valid iterable value.");

    CompileType* elementType = (*typeArgs)[0];

    GEN_FAIL("List gen not implemented.");


}

CompileVal* AstWalker::codeGen_initial(Json::Value jsonNode)
{
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

    //If none of the TRY_NODE blocks returned anything, then we have an unimplemented ast node.
    cout << jsonNode << endl;
    GEN_FAIL("Unimplemented node type in code generator");

    return nullptr;
}

void AstWalker::codeGen_top(std::string jsonString)
{
    Json::Value jsonNode = generateFromJson(jsonString);
    codeGen_initial(jsonNode);
}

Json::Value AstWalker::generateFromJson(std::string jsonString)
{
    Json::Reader reader;
    Json::Value  json_root;
    reader.parse(jsonString, json_root);
    return json_root;
}

CompileVal* AstWalker::createConstObject(CompileType::CommonType commonType,llvm::Value* raw_value)
{
    return createConstObject(CompileType::getCommonTypeName(commonType), raw_value);
}

CompileVal* AstWalker::createConstObject(std::string typeName, llvm::Value* raw_value)
{
    std::string init_func_name = "init_" + typeName;

    llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
    llvm::Value* result             = createNativeCall(init_func_name, {raw_value});

    Builder.SetInsertPoint(originalBlock);

    return new CompileVal(result, typeName);
}

llvm::Module* AstWalker::getModule() { return currModule.get(); }

llvm::LLVMContext* AstWalker::getContext() { return &currContext; }

int main()
{
    AstWalker* ast = new AstWalker("teeeest", "../stdlib/stdlib.ll");
    std::string jsonString;

    if( std::cin ) 
    {
        getline(std::cin, jsonString);
        ast->codeGen_top(jsonString);
        CodeGenUtil::dumpIR(ast->getModule());
        CodeGenUtil::writeToFile("../Parser/test.bc", ast->getModule());
    }
}
