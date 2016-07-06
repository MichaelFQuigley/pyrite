#include <iostream>
#include <cstdlib>
#include "ast_walker.h"
#include "codegen_util.h"

using namespace std;

llvm::Type* AstWalker::getVoidStarType()
{
    return llvm::PointerType::get(llvm::Type::getInt8Ty(currContext), 0);
}

AstWalker::AstWalker(std::string filename, std::string stdlib_filename) : Builder(currContext)
{

    currModule  = llvm::make_unique<llvm::Module>(filename, currContext);
    scopeHelper = new ScopeHelper();
    if( !CodeGenUtil::load_stdlib(stdlib_filename, currModule.get(), &currContext) )
    {
        return;
    }

    //TODO automate this process...
    CompileVal* printlnVal = new CompileVal(tryGetFunction("println"), new CompileType("Function"));
    printlnVal->setArgumentsList(new std::vector<CompileType*>({new CompileType("String"), new CompileType("Void")}));
    scopeHelper->setNamedVal("println", printlnVal, true);

    CompileVal* String_IntVal = new CompileVal(tryGetFunction("String_Int"), new CompileType("Function"));
    String_IntVal->setArgumentsList(new std::vector<CompileType*>({new CompileType("Int"), new CompileType("String")}));
    scopeHelper->setNamedVal("String_Int", String_IntVal, true);
}

bool AstWalker::json_node_has(Json::Value json_node, std::string name, Json::Value* out_node)
{
    if( json_node[name] != Json::nullValue )
    {
        *out_node = json_node[name];
        return true;
    }

    return false;
}

CompileVal* AstWalker::codeGen_StmtsOp(Json::Value json_node)
{
    CompileVal* result = nullptr;
    for( unsigned i = 0; i < json_node.size(); i++ )
    {
        result = codeGen_initial(json_node[i]);
    }

    return result;
}

llvm::StructType* AstWalker::getTypeFromStr(std::string typeName)
{
    llvm::StructType* type = currModule->getTypeByName("struct." + typeName);
    GEN_ASSERT(type != nullptr, "Unknown type.");

    return type;
}

llvm::Type* AstWalker::getPtrTypeFromStr(std::string typeName)
{
    if( typeName == "Void" )
    {
        return llvm::Type::getVoidTy(currContext);
    }

    llvm::StructType* type = getTypeFromStr(typeName);

    return llvm::PointerType::getUnqual(type);
}

void AstWalker::pushScope(ScopeNode::ScopeType scopeType, bool funcScopeRetVoid)
{
    scopeHelper->pushScope(scopeType, funcScopeRetVoid);
}

void AstWalker::popScope()
{
    scopeHelper->popScope();

}

CompileType* AstWalker::makeCompileType(Json::Value json_node)
{
    CompileType* result = nullptr;
    if( json_node["simple"] != Json::nullValue )
    {
        result = new CompileType(json_node["simple"].asString());
    }
    else if( json_node["list_type"] != Json::nullValue )
    {
        result =  new CompileType("List"); 
        result->insertArgumentsList(makeCompileType(json_node["list_type"]));
    }
    else
    {
        cout << json_node << endl;
        GEN_ASSERT(false, "Unimplemented compile type for makeCompileType");
    }

    return result;
}

CompileVal* AstWalker::makeFuncProto(Json::Value json_node)
{
    std::vector<llvm::Type*> argsV;
    Json::Value header_node                = json_node["header"]["FuncProto"];
    Json::Value args_node                  = header_node["args"];
    std::vector<CompileType*>* compileArgs = new std::vector<CompileType*>();

    for( const Json::Value& val : args_node )
    {
        argsV.push_back(getVoidStarType());
        compileArgs->push_back(makeCompileType(val["TypedArg"]["type"]));
    }
    CompileType* compileRetType = makeCompileType(header_node["ret_type"]);
    compileArgs->push_back(compileRetType); 
    std::string funcName          = json_node["name"].asString();
    llvm::Type* ret_type          = compileRetType->getTypeName() == "Void" ?  
                                        llvm::Type::getVoidTy(currContext)
                                        : getVoidStarType();
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

    CompileVal* result          = new CompileVal(func, "Function"); 
    result->setArgumentsList(compileArgs);

    return result;
}

CompileVal* AstWalker::codeGen_ExprOp(Json::Value json_node){ return codeGen_initial(json_node); }

CompileVal* AstWalker::codeGen_VarDef(Json::Value json_node){ 
    std::string varName   = json_node["var"]["TypedArg"]["name"].asString();
    CompileVal* rhs_expr = codeGen_initial(json_node["expr"]); 

    GEN_ASSERT(rhs_expr != nullptr, "Invalid assignment to rhs of variable definition.");

    newVarInScope(varName, rhs_expr); 

    return nullptr; 
}

CompileVal* AstWalker::newVarInScope(std::string varName, CompileVal* value)
{
    llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
    llvm::BasicBlock& func_block    = originalBlock->getParent()->getEntryBlock();
    llvm::Value* allocaRes;
    
    Builder.SetInsertPoint(&func_block);
    allocaRes = Builder.CreateAlloca(getVoidStarType());
    Builder.SetInsertPoint(originalBlock);
    Builder.CreateStore(value->getRawValue(), allocaRes);
    CompileVal* result = new CompileVal(allocaRes, value->getCompileType());
    scopeHelper->setNamedVal(varName, result, true);
   
    return result;
}

CompileVal* AstWalker::codeGen_BinOp(Json::Value json_node){
    std::string op = json_node["op"].asString();
    std::string op_func_prefix = "";

    if( op == "=" ) {
        Json::Value lhs_val;
        //lhs must be variable for now
        if(json_node_has(json_node["lhs"], "AtomOp", &lhs_val)
            && json_node_has(lhs_val, "Id", &lhs_val)
            && lhs_val["trailers"].size() == 0 )
        {
            std::string var_name = lhs_val["name"].asString();
            CompileVal* var_val = scopeHelper->getNamedVal(var_name, true);
            GEN_ASSERT(var_val != nullptr, "No variable avaiable");
            
            CompileVal* rhs_val = codeGen_initial(json_node["rhs"]);

            GEN_ASSERT(rhs_val != nullptr, "Invalid expression on rhs of assignment.");

            llvm::Value* result_store = Builder.CreateStore(rhs_val->getRawValue(), var_val->getRawValue());
            uint64_t varIndex         = scopeHelper->getNamedValInd(var_name);

            std::vector<llvm::Value*> argsV;
            argsV.push_back(Builder.CreateLoad(var_val->getRawValue()));
            argsV.push_back(CodeGenUtil::getConstInt64(&currContext, varIndex, false));
            createNativeCall("gc_set_named_var_in_scope", argsV);

            return new CompileVal(result_store, rhs_val->getCompileType());
        }
        else
        {
            GEN_ASSERT(false, "Lhs of assignment must be variable.");
        }
    }
    else if( op == "+" )  { op_func_prefix = "add";}
    else if( op == "-" )  { op_func_prefix = "sub";}
    else if( op == "*" )  { op_func_prefix = "mul";}
    else if( op == "/" )  { op_func_prefix = "div";}
    else if( op == "%" )  { op_func_prefix = "mod";}
    else if( op == "&" )  { op_func_prefix = "and";}
    else if( op == "|" )  { op_func_prefix = "or";}
    else if( op == "^" )  { op_func_prefix = "xor";}
    else if( op == "<" )  { op_func_prefix = "cmplt";}
    else if( op == "<=" ) { op_func_prefix = "cmple";}
    else if( op == "==" ) { op_func_prefix = "cmpeq";}
    else if( op == ">" )  { op_func_prefix = "cmpgt";}
    else if( op == ">=" ) { op_func_prefix = "cmpge";}
    else if( op == "!=" ) { op_func_prefix = "cmpne";}
    else { 
        cout << "Unimplemented operator type" << endl;
        return nullptr;
    }

    CompileVal* lhs          = codeGen_initial(json_node["lhs"]);
    std::string op_func_name = op_func_prefix + "_" + lhs->getCompileType()->getTypeName();
    CompileVal* rhs          = codeGen_initial(json_node["rhs"]); 

    std::vector<llvm::Value *> argsV;
    argsV.push_back(lhs->getRawValue());
    argsV.push_back(rhs->getRawValue());

    llvm::Value* result_val = createNativeCall(op_func_name, argsV);

    //TODO change so that lhs type isn't blindly used
    return new CompileVal(result_val, lhs->getCompileType());
}

CompileVal* AstWalker::codeGen_AtomOp(Json::Value json_node) { 
    Json::Value val_node = Json::nullValue;

    if( json_node_has(json_node, "Lit", &val_node) ) 
    {
        std::string lit_str = val_node.asString();
        std::string lit_val = lit_str.substr(1);
        char lit_type       = lit_str[0];

        double double_val;
        long int_val;
        switch( lit_type )
        {
            case 'f':
                double_val = stod(lit_val);
                return createConstObject("Float", llvm::ConstantFP::get(currContext, llvm::APFloat(double_val)));
            case 'i':
                int_val = stol(lit_val);
                return createConstObject("Int", CodeGenUtil::getConstInt64(&currContext, int_val));
            case 's':
                 return createConstObject("String", 
                                    CodeGenUtil::generateString(currModule.get(), lit_val));
            case 'b':
                int_val = (lit_val == "true") ? 1 : 0;
                return createConstObject("Bool", llvm::ConstantInt::get(currContext, llvm::APInt(1, int_val, false)));
            default:
                 cout << "Unimplemented literal type" << endl;
                break;
        }
    }
    else if( json_node_has(json_node, "Id", &val_node) )
    {
        std::string id_name  = val_node["name"].asString();
        Json::Value trailers = val_node["trailers"];
        CompileVal* result   = nullptr;

        CompileVal* var_val = scopeHelper->getNamedVal(id_name, true);
        //TODO change how functions are stored
        //
        //Function pointers and other variables need to be loaded
        if( var_val->getCompileType()->getTypeName() != "Function" )
        {
            result = new CompileVal(Builder.CreateLoad(var_val->getRawValue()), var_val->getCompileType());
        }
        //regular functions do not need to be loaded
        else
        {
            result = new CompileVal(var_val->getRawValue(), var_val->getCompileType());
        }
        for( int i = 0; i < trailers.size(); i++ )
        {
            if( trailers[i]["FCall"] != Json::nullValue )
            {
                std::vector<CompileVal*>* fcallArgs = new std::vector<CompileVal*>();
                for(const Json::Value& val : trailers[i]["FCall"]["args"])
                {
                    fcallArgs->push_back(codeGen_initial(val));
                }
                
                result = createLangCall(result, fcallArgs);               
            }
            else if( trailers[i]["Index"] != Json::nullValue )
            {
                std::vector<llvm::Value*> argsV;
                CompileVal* indexVal    = codeGen_initial(trailers[i]["Index"]);
                argsV.push_back(result->getRawValue());
                argsV.push_back(indexVal->getRawValue());
                std::string lhsTypename = result->getCompileType()->getTypeName();
                //The type of return value from the call to 'get_*' will always be the 
                //first argument in the compile type's argument list
                CompileType* returnedType = (*(result->getCompileType()->getArgumentsList()))[0];
                result = new CompileVal(createNativeCall("get_" + lhsTypename, argsV), returnedType);
            }
        }

        return result;
    }
    else if( json_node_has(json_node, "ParenExpr", &val_node) )
    {
        return codeGen_ExprOp(val_node);
    }
    else if( json_node_has(json_node, "RangeOp", &val_node) )
    {
        CompileVal* start = codeGen_AtomOp(val_node["start"]);
        CompileVal* step  = createConstObject("Int", 
                                                llvm::ConstantInt::get(currContext, 
                                                llvm::APInt(64, 1, true)));
        CompileVal* end   = codeGen_AtomOp(val_node["end"]);

        std::vector<llvm::Value*> argsV;
        argsV.push_back(start->getRawValue());
        argsV.push_back(step->getRawValue());
        argsV.push_back(end->getRawValue());
        return new CompileVal(createNativeCall("init_IntRange", argsV), "Int");
    }
    else
    {
        GEN_ASSERT(false, "Unimplemented atom value.");
    }
    return nullptr;
}

llvm::BasicBlock* AstWalker::makeBasicBlock(std::string name)
{
    return llvm::BasicBlock::Create(currContext, 
                                    name, 
                                    Builder.GetInsertBlock()->getParent());
}

std::string AstWalker::createConstructorName(std::string func_name, 
        std::vector<llvm::Value*> argsV)
{
    std::string full_func_name = func_name;

    for(auto &argI : argsV)
    {
        full_func_name += "_" + CodeGenUtil::getTypeStr(argI, false);
    }

    return full_func_name;
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
    GEN_ASSERT(func->getCompileType()->getTypeName() == "Function", "Cannot call function!");
    std::vector<llvm::Value*> nativeArgs;
   /* GEN_ASSERT(argsV->size() == funcArgs->size() - 1, 
            "Number of arguments in function call " +  funcName + " must match.\n"
            + "expected " + to_string(funcArgs->size())  
            + "arguments, but received " + to_string(argsV->size()));*/

    std::vector<CompileType*>* argumentsList = (func->getCompileType()->getArgumentsList());
    //last argument in CompileVal is return type
    CompileType* retType = (*argumentsList)[argumentsList->size() - 1];

    for(int i = 0; i < argsV->size(); i++)
    {
        //TODO recursive type assertions
      //  GEN_ASSERT((*argsV)[i]->getCompileType()->getTypeName()
             //       == (*funcArgs)[i]->getTypeName(), 
              //    "Types in function call must match");
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
    std::vector<llvm::Value*> argsV;
    argsV.push_back(Bool);
    llvm::Value* raw_bool = createNativeCall("rawVal_Bool", argsV);
    argsV.clear();

    Builder.CreateCondBr(raw_bool, trueBlock, falseBlock);
}

CompileVal* AstWalker::codeGen_ForOp(Json::Value json_node){ 
    std::vector<llvm::Value*> argsV;
    llvm::BasicBlock* loopTop    = makeBasicBlock("loopTop");
    llvm::BasicBlock* loopBody   = llvm::BasicBlock::Create(currContext, "loopBody");
    llvm::BasicBlock* loopBottom = llvm::BasicBlock::Create(currContext, "loopBottom");

    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);

    CompileVal* itt                = codeGen_AtomOp(json_node["itt"]);
    //XXX only supports int iterators for now
    std::string itt_hasNextFuncName = "hasNext_IntRange";
    std::string itt_nextFuncName    = "next_IntRange";
    std::string itt_beginFuncName   = "begin_IntRange";
    std::string loop_var_name       = json_node["loop_var"].asString();

    argsV.push_back(itt->getRawValue());
    llvm::Value* loop_var = createNativeCall(itt_beginFuncName, argsV);
    argsV.clear();
    //XXX change from 'Int' when possible
    newVarInScope(loop_var_name, new CompileVal(loop_var, "Int"));

    Builder.CreateBr(loopTop);
    Builder.SetInsertPoint(loopTop);

    argsV.push_back(itt->getRawValue());
    llvm::Value* hasNext          = createNativeCall(itt_hasNextFuncName, argsV);
    argsV.clear();

    createBoolCondBr(hasNext, loopBody, loopBottom);

    startBlock(loopBody);
    
    createNativeCall("gc_push_loop_scope", argsV);
    argsV.clear();

    codeGen_initial(json_node["body"]);

    argsV.push_back(itt->getRawValue());
    Builder.CreateStore(createNativeCall(itt_nextFuncName, argsV), scopeHelper->getNamedVal(loop_var_name, true)->getRawValue());
    argsV.clear();

    createNativeCall("gc_pop_scope", argsV);
    argsV.clear();
    Builder.CreateBr(loopTop);
    startBlock(loopBottom);

    popScope();

    return nullptr;
}

CompileVal* AstWalker::codeGen_WhileOp(Json::Value json_node)
{ 
    llvm::BasicBlock* loopTop    = llvm::BasicBlock::Create(currContext, "whileTop");
    llvm::BasicBlock* loopBody   = llvm::BasicBlock::Create(currContext, "whileBody");
    llvm::BasicBlock* loopBottom = llvm::BasicBlock::Create(currContext, "whileBottom");

    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
    
    Builder.CreateBr(loopTop);
    startBlock(loopTop);

    CompileVal* header = codeGen_initial(json_node["header"]);
    createBoolCondBr(header->getRawValue(), loopBody, loopBottom);

    startBlock(loopBody);
    codeGen_initial(json_node["body"]);

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

CompileVal* AstWalker::codeGen_IfOp(Json::Value json_node)
{
    CompileVal* test = codeGen_initial(json_node["test"]);
    std::vector<llvm::Value*> argsV;
    argsV.push_back(test->getRawValue());
    
    llvm::BasicBlock* ifTrue = makeBasicBlock("ifTrue");
    llvm::BasicBlock* endIf;

    CompileVal* result = nullptr;
    //check to see if there is just an if block without an else
    if( json_node["bodies"].size() == 1 )
    {
        pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
        endIf = llvm::BasicBlock::Create(currContext, "endif");
        createBoolCondBr(test->getRawValue(), ifTrue, endIf);
        Builder.SetInsertPoint(ifTrue);
        codeGen_initial(json_node["bodies"][0]);
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
        CompileVal* ifTrueLastStmt = codeGen_initial(json_node["bodies"][0]);
        popScope();
        //if false codegen
        pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
        Builder.CreateBr(endIf);
        ifTrue = Builder.GetInsertBlock();
        startBlock(ifFalse);
        CompileVal* ifFalseLastStmt = codeGen_initial(json_node["bodies"][1]);
        Builder.CreateBr(endIf);
        ifFalse = Builder.GetInsertBlock();
        startBlock(endIf);
        popScope();
        //phi node
        if( ifTrueLastStmt != nullptr && ifFalseLastStmt != nullptr )
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

llvm::Value* AstWalker::createGlobalFunctionConst(std::string funcName, llvm::Function* initValue, CompileType* functionType)
{
    ScopeNode* globalScope       = scopeHelper->getNearestScopeOfType(ScopeNode::ScopeType::TOP_SCOPE);
    llvm::GlobalVariable* result = new llvm::GlobalVariable(
                                            *currModule.get(), 
                                            getVoidStarType(), 
                                            false,
                                            llvm::GlobalValue::PrivateLinkage,
                                            initValue,
                                            funcName);
    globalScope->setNamedVal(funcName, new CompileVal(result, functionType), 0);    
    return result;

}

CompileVal* AstWalker::codeGen_FuncDef(Json::Value json_node)
{
    if( scopeHelper->getCurrScopeType() == ScopeNode::ScopeType::TOP_SCOPE )
    {
        pushScope(ScopeNode::ScopeType::FUNC_SCOPE);

        CompileVal* compileFunc = makeFuncProto(json_node);
        llvm::Function* func       = static_cast<llvm::Function*>(compileFunc->getRawValue());
        llvm::BasicBlock* entry    = llvm::BasicBlock::Create(currContext, "varDecls", func);
        llvm::BasicBlock* funcBody = llvm::BasicBlock::Create(currContext, "funcBody");
        std::string funcName       = json_node["name"].asString();
        scopeHelper->setBlockOnCurrScope(entry);
        Builder.SetInsertPoint(entry);
        scopeHelper->setNamedVal(funcName, compileFunc, true);

        if( funcName == "main" )
        {
            std::vector<llvm::Value*> argsV;
            createNativeCall("initialize_core",argsV);
        }

        int arg_index = 0;
        for(auto argI = func->arg_begin(); arg_index < ((int)func->arg_size()); argI++, arg_index++)
        {
            newVarInScope(argI->getName(), 
                    new CompileVal(argI, 
                        makeCompileType(json_node["header"]
                                                 ["FuncProto"]
                                                 ["args"]
                                                 [arg_index]
                                                 ["TypedArg"]
                                                 ["type"])));
        }

        startBlock(funcBody);
        
        codeGen_initial(json_node["stmts"]);

        Builder.SetInsertPoint(entry);
        Builder.CreateBr(funcBody);
        Builder.SetInsertPoint(funcBody);

        llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
        llvm::BasicBlock& func_block    = originalBlock->getParent()->getEntryBlock();

        uint64_t numVarsInFunc = scopeHelper->getNumNamedVarsSinceFunc();
        Builder.SetInsertPoint(&func_block);
        std::vector<llvm::Value*> argsV;
        argsV.push_back(CodeGenUtil::getConstInt64(&currContext, numVarsInFunc, false));
        createNativeCall("gc_push_func_scope", argsV); 
        Builder.SetInsertPoint(originalBlock);

        popScope();
       
        scopeHelper->setNamedVal(funcName, compileFunc, true);

        return compileFunc; 
    }
    else
    {
        GEN_ASSERT(false, "Nested functions are not yet implemented");
    }
}

CompileVal* AstWalker::codeGen_ReturnOp(Json::Value json_node)
{ 
    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);

    CompileVal* ret_val             = codeGen_initial(json_node);
    llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
    llvm::Function* func_block      = originalBlock->getParent();
    //check for return param
    popScope();
    std::vector<llvm::Value*> argsV;
    createNativeCall("gc_pop_scope", argsV); 
    
    if( ret_val == nullptr )
    {
        return new CompileVal(Builder.CreateRetVoid(), "Void");
    }
    else
    {
        return new CompileVal(Builder.CreateRet(ret_val->getRawValue()), ret_val->getCompileType()); 
    }

}

CompileVal* AstWalker::codeGen_ListOp(Json::Value json_node)
{

    std::vector<llvm::Value*> argsV;
    int numListItems = json_node.size();
    CompileVal* list = createConstObject("List", CodeGenUtil::getConstInt64(&currContext, numListItems, false));
    for( unsigned i = 0; i < json_node.size(); i++ )
    {
        argsV.clear();
        CompileVal* list_el = codeGen_initial(json_node[i]);

        if( i == 0 )
        {
            list->insertArgumentType(list_el->getCompileType());
        }

        argsV.push_back(list->getRawValue());
        // index 
        argsV.push_back(createConstObject("Int", CodeGenUtil::getConstInt64(&currContext, i, false))->getRawValue());
        // value 
        argsV.push_back(list_el->getRawValue());
        createNativeCall("set_List", argsV);
    }

    return list;   
}

CompileVal* AstWalker::AstWalker::codeGen_initial(Json::Value json_node)
{
    TRY_NODE(json_node, StmtsOp);
    TRY_NODE(json_node, ExprOp);
    TRY_NODE(json_node, VarDef);
    TRY_NODE(json_node, BinOp);
    TRY_NODE(json_node, AtomOp);
    TRY_NODE(json_node, ForOp);
    TRY_NODE(json_node, WhileOp);
    TRY_NODE(json_node, IfOp);
    TRY_NODE(json_node, FuncDef);
    TRY_NODE(json_node, ReturnOp);
    TRY_NODE(json_node, ListOp);

    //If none of the TRY_NODE blocks returned anything, then we have an unimplemented ast node.
    cout << json_node << endl;
    GEN_ASSERT(false, "Unimplemented node type in code generator");

    return nullptr;
}

void AstWalker::codeGen_top(std::string json_string)
{
    Json::Value json_node = generateFromJson(json_string);
    codeGen_initial(json_node);
}

Json::Value AstWalker::generateFromJson(std::string json_string)
{
    Json::Reader reader;
    Json::Value  json_root;
    reader.parse(json_string, json_root);
    return json_root;
}

CompileVal* AstWalker::createConstObject(std::string type_name, llvm::Value* raw_value)
{
    std::string init_func_name = "init_" + type_name;
    std::vector<llvm::Value *> argsV;

    argsV.push_back(raw_value);

    llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
    llvm::Value* result             = createNativeCall(init_func_name, argsV);

    Builder.SetInsertPoint(originalBlock);

    return new CompileVal(result, type_name);
}

llvm::Module* AstWalker::getModule() { return currModule.get(); }

llvm::LLVMContext* AstWalker::getContext() { return &currContext; }

int main()
{
    AstWalker* ast = new AstWalker("teeeest", "../CodeGen/stdlib/stdlib.ll");
    std::string json_string;

    if( std::cin ) 
    {
        getline(std::cin, json_string);
        ast->codeGen_top(json_string);
        CodeGenUtil::dumpIR(ast->getModule());
        CodeGenUtil::writeToFile("../Lexers/test.bc", ast->getModule());
    }
}
