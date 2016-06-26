#include <iostream>
#include <cstdlib>
#include "ast_walker.h"
#include "codegen_util.h"

using namespace std;

AstWalker::AstWalker(std::string filename, std::string stdlib_filename) : Builder(currContext)
{

    currModule  = llvm::make_unique<llvm::Module>(filename, currContext);
    scopeHelper = new ScopeHelper();

    if( !CodeGenUtil::load_stdlib(stdlib_filename, currModule.get(), &currContext) )
    {
        return;
    }
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

llvm::Value* AstWalker::codeGen_StmtsOp(Json::Value json_node)
{
    llvm::Value* result = nullptr;
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

llvm::Function* AstWalker::makeFuncProto(Json::Value json_node, std::string result_param_name)
{
    std::vector<llvm::Type*> argsV;
    Json::Value header_node = json_node["header"]["FuncProto"];
    Json::Value retTypeNode = header_node["ret_type"];

     GEN_ASSERT(retTypeNode["simple"], 
             "Simples are the only types supported at the moment");

    std::string retTypeName = retTypeNode["simple"].asString();
    llvm::Type* retType     = getPtrTypeFromStr(retTypeName);

    scopeHelper->setParentFuncReturnsVoid(retType->isVoidTy());

    for( const Json::Value& val : header_node["args"] )
    {
        Json::Value typedArg = val["TypedArg"];
        Json::Value argType  = typedArg["type"];

        GEN_ASSERT(argType["simple"], 
                "Simples are the only types supported at the moment");

        std::string argTypeName = argType["simple"].asString();
        
        argsV.push_back(getPtrTypeFromStr(argTypeName));
    }
   
    std::string funcName          = json_node["name"].asString();
    llvm::FunctionType* funcProto = llvm::FunctionType::get(retType, 
                                                                argsV, 
                                                                false);    
    llvm::Function* func          = llvm::Function::Create(funcProto, 
                                                            llvm::Function::ExternalLinkage, 
                                                            funcName,
                                                            currModule.get());
    {
        int arg_index = 0;
                           
        for(auto argI = func->arg_begin(); 
                arg_index < ((int)func->arg_size()); 
                argI++, arg_index++)
        {
            argI->setName(header_node["args"][arg_index]["TypedArg"]["name"].asString());
        }
    }

    return func; 
}

llvm::Value* AstWalker::codeGen_ExprOp(Json::Value json_node){ return codeGen_initial(json_node); }

llvm::Value* AstWalker::codeGen_VarDef(Json::Value json_node){ 
    std::string varName   = json_node["var"]["TypedArg"]["name"].asString();
    llvm::Value* rhs_expr = codeGen_initial(json_node["expr"]); 

    newVarInScope(varName, rhs_expr); 

    return nullptr; 
}

llvm::Value* AstWalker::newVarInScope(std::string varName, llvm::Value* value)
{
    llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
    llvm::BasicBlock& func_block    = originalBlock->getParent()->getEntryBlock();
    llvm::Value* allocaRes;
    
    Builder.SetInsertPoint(&func_block);
    allocaRes = Builder.CreateAlloca(value->getType());
    Builder.SetInsertPoint(originalBlock);
    Builder.CreateStore(value, allocaRes);
    scopeHelper->setNamedVal(varName, allocaRes, true);
   
    return allocaRes;
}

llvm::Value* AstWalker::codeGen_BinOp(Json::Value json_node){
    std::string op = json_node["op"].asString();
    std::string op_func_prefix = "";
    

    if( op == "=" ) {
        Json::Value lhs_val;
        //lhs must be variable for now
        if(json_node_has(json_node["lhs"], "AtomOp", &lhs_val)
            && json_node_has(lhs_val, "Variable", &lhs_val))
        {
            std::string var_name = lhs_val.asString();
            llvm::Value* var_val = scopeHelper->getNamedVal(var_name, true);
            GEN_ASSERT(var_val != nullptr, "No variable avaiable");

            llvm::Value* result = Builder.CreateStore(codeGen_initial(json_node["rhs"]), var_val);
            uint64_t varIndex   = scopeHelper->getNamedValInd(var_name);
            std::vector<llvm::Value*> argsV;
            argsV.push_back(Builder.CreatePointerCast(Builder.CreateLoad(var_val), 
                        llvm::PointerType::get(llvm::Type::getInt8Ty(currContext), 0)));
            llvm::Value* obj_back_ptr = createCall("get_back_ptr", argsV);
            argsV.clear();

            argsV.push_back(obj_back_ptr);
            argsV.push_back(llvm::ConstantInt::get(currContext, llvm::APInt(64, 
                                                                    varIndex, 
                                                                    false)));
            createCall("gc_set_named_var_in_scope", argsV);

            return result;
        }
        else
        {
            GEN_ASSERT(false, "Lhs of assignment must be variable.");
        }
    }
    else if( op == "+" )  { op_func_prefix = "add_";}
    else if( op == "-" )  { op_func_prefix = "sub_";}
    else if( op == "*" )  { op_func_prefix = "mul_";}
    else if( op == "/" )  { op_func_prefix = "div_";}
    else if( op == "%" )  { op_func_prefix = "mod_";}
    else if( op == "&" )  { op_func_prefix = "and_";}
    else if( op == "|" )  { op_func_prefix = "or_";}
    else if( op == "^" )  { op_func_prefix = "xor_";}
    else if( op == "<" )  { op_func_prefix = "cmplt_";}
    else if( op == "<=" ) { op_func_prefix = "cmple_";}
    else if( op == "==" ) { op_func_prefix = "cmpeq_";}
    else if( op == ">" )  { op_func_prefix = "cmpgt_";}
    else if( op == ">=" ) { op_func_prefix = "cmpge_";}
    else if( op == "!=" ) { op_func_prefix = "cmpne_";}
    else { 
        cout << "Unimplemented operator type" << endl;
        return nullptr;
    }

    llvm::Value* lhs = codeGen_initial(json_node["lhs"]); 
    llvm::Value* rhs = codeGen_initial(json_node["rhs"]); 

    std::vector<llvm::Value *> argsV;
    argsV.push_back(lhs);
    argsV.push_back(rhs);

    std::string type_str = CodeGenUtil::getTypeStr(lhs, false);
    std::string op_funcname = op_func_prefix + type_str; 
    llvm::Value* result_val = createCall(op_funcname, argsV);

    return result_val;
}

llvm::Value* AstWalker::codeGen_AtomOp(Json::Value json_node) { 
    Json::Value val_node = Json::nullValue;

    if( json_node_has(json_node, "Lit", &val_node) ) 
    {
        std::string lit_str = val_node.asString();
        std::string lit_val = lit_str.substr(1);
        char lit_type = lit_str[0];

        double double_val;
        long int_val;
        switch( lit_type )
        {
            case 'f':
                double_val = stod(lit_val);
                return createConstObject("Float", 
                                  llvm::ConstantFP::get(currContext, 
                                      llvm::APFloat(double_val)));
                break;
            case 'i':
                int_val = stol(lit_val);
                return createConstObject("Int", 
                                  llvm::ConstantInt::get(currContext, 
                                      llvm::APInt(64, int_val, true)));
                break;
            case 's':
                 return createConstObject("String", 
                                    CodeGenUtil::generateString(currModule.get(), lit_val));
            case 'b':
                int_val = (lit_val == "true") ? 1 : 0;
                return createConstObject("Bool", 
                                  llvm::ConstantInt::get(currContext, 
                                      llvm::APInt(1, int_val, false)));
            default:
                 cout << "Unimplemented literal type" << endl;
                break;
        }
    }
    else if( json_node_has(json_node, "FCall", &val_node) ) 
    {
        std::string func_name = val_node["name"].asString();
        std::vector<llvm::Value*> argsV;
        for(const Json::Value& val : val_node["args"])
        {
            argsV.push_back(codeGen_initial(val));
        }

        return createCall(func_name, argsV);
    }
    else if( json_node_has(json_node, "Variable", &val_node) ) 
    {
        std::string var_name = val_node.asString();
        llvm::Value* var_val = scopeHelper->getNamedVal(var_name, true);

        if( var_val == nullptr )
        {
            cout << "Undefined variable " << var_name << endl;
            throw;
        }

        return Builder.CreateLoad(var_val);
    }
    else if( json_node_has(json_node, "ParenExpr", &val_node) )
    {
        return codeGen_ExprOp(val_node);
    }
    else if( json_node_has(json_node, "RangeOp", &val_node) )
    {
        llvm::Value* start = codeGen_AtomOp(val_node["start"]);
        llvm::Value* step  = createConstObject("Int", 
                                                llvm::ConstantInt::get(currContext, 
                                                llvm::APInt(64, 1, true)));
        llvm::Value* end   = codeGen_AtomOp(val_node["end"]);

        std::vector<llvm::Value*> argsV;
        argsV.push_back(start);
        argsV.push_back(step);
        argsV.push_back(end);
        return createCall("init_IntRange", argsV);
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

llvm::Value* AstWalker::createCall(std::string func_name, 
        std::vector<llvm::Value*> argsV, 
        bool restore_insert_point)
{
    std::string full_func_name = func_name; 

    //Upper case function call is by convention a constructor call
    if( isupper(func_name[0]) )
    {
        full_func_name = createConstructorName(func_name, argsV);
    }

    auto func            = tryGetFunction(full_func_name);
    llvm::Value* ret_val = Builder.CreateCall(func, argsV);

    return ret_val;
}

void AstWalker::createBoolCondBr(llvm::Value* Bool, 
        llvm::BasicBlock* trueBlock,
        llvm::BasicBlock* falseBlock)
{
    std::vector<llvm::Value*> argsV;
    argsV.push_back(Bool);
    llvm::Value* raw_bool = createCall("rawVal_Bool", argsV);
    argsV.clear();

    Builder.CreateCondBr(raw_bool, trueBlock, falseBlock);
}

llvm::Value* AstWalker::codeGen_ForOp(Json::Value json_node){ 
    std::vector<llvm::Value*> argsV;
    llvm::BasicBlock* loopTop    = makeBasicBlock("loopTop");
    llvm::BasicBlock* loopBody   = llvm::BasicBlock::Create(currContext, "loopBody");
    llvm::BasicBlock* loopBottom = llvm::BasicBlock::Create(currContext, "loopBottom");

    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);

    llvm::Value* itt                = codeGen_AtomOp(json_node["itt"]);
    //XXX only supports int iterators for now
    std::string itt_hasNextFuncName = "hasNext_IntRange";
    std::string itt_nextFuncName    = "next_IntRange";
    std::string itt_beginFuncName   = "begin_IntRange";
    std::string loop_var_name       = json_node["loop_var"]["Variable"].asString();

    argsV.push_back(itt);
    llvm::Value* loop_var           = createCall(itt_beginFuncName, argsV);
    argsV.clear();

    newVarInScope(loop_var_name, loop_var);

    Builder.CreateBr(loopTop);
    Builder.SetInsertPoint(loopTop);

    argsV.push_back(itt);
    llvm::Value* hasNext          = createCall(itt_hasNextFuncName, argsV);
    argsV.clear();

    createBoolCondBr(hasNext, loopBody, loopBottom);

    startBlock(loopBody);
    codeGen_initial(json_node["body"]);

    argsV.push_back(itt);
    //loop_var = createCall(itt_nextFuncName, argsV);
    //llvm::Value* loaded_loop_var = Builder.CreateLoad(loop_var);
    Builder.CreateStore(createCall(itt_nextFuncName, argsV), scopeHelper->getNamedVal(loop_var_name, true));
    argsV.clear();

    Builder.CreateBr(loopTop);
    startBlock(loopBottom);

    popScope();

    return nullptr;
}

llvm::Value* AstWalker::codeGen_WhileOp(Json::Value json_node)
{ 
    llvm::BasicBlock* loopTop    = llvm::BasicBlock::Create(currContext, "whileTop");
    llvm::BasicBlock* loopBody   = llvm::BasicBlock::Create(currContext, "whileBody");
    llvm::BasicBlock* loopBottom = llvm::BasicBlock::Create(currContext, "whileBottom");

    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
    
    Builder.CreateBr(loopTop);
    startBlock(loopTop);

    llvm::Value* header = codeGen_initial(json_node["header"]);
    createBoolCondBr(header, loopBody, loopBottom);

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

llvm::Value* AstWalker::codeGen_IfOp(Json::Value json_node)
{
    llvm::Value* test = codeGen_initial(json_node["test"]);
    std::vector<llvm::Value*> argsV;
    argsV.push_back(test);
    
    llvm::BasicBlock* ifTrue = makeBasicBlock("ifTrue");
    llvm::BasicBlock* endIf;

    llvm::Value* result = nullptr;
    //check to see if there is an else block
    if( json_node["bodies"].size() == 1 )
    {
        pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
        endIf = llvm::BasicBlock::Create(currContext, "endif");
        createBoolCondBr(test, ifTrue, endIf);
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
        createBoolCondBr(test, ifTrue, ifFalse);
        //if true codegen
        pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
        Builder.SetInsertPoint(ifTrue);
        llvm::Value* ifTrueLastStmt = codeGen_initial(json_node["bodies"][0]);
        popScope();
        //if false codegen
        pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);
        Builder.CreateBr(endIf);
        ifTrue = Builder.GetInsertBlock();
        startBlock(ifFalse);
        llvm::Value* ifFalseLastStmt = codeGen_initial(json_node["bodies"][1]);
        Builder.CreateBr(endIf);
        ifFalse = Builder.GetInsertBlock();
        startBlock(endIf);
        popScope();
        //phi node
        CodeGenUtil::assertType(ifTrueLastStmt, 
                ifFalseLastStmt, 
                "Types at end of if-else must match");
        llvm::PHINode * phi = Builder.CreatePHI(ifTrueLastStmt->getType(), 2, "ifPhi");
        phi->addIncoming(ifTrueLastStmt, ifTrue);
        phi->addIncoming(ifFalseLastStmt, ifFalse);
        result = phi;
    }

    return result;
}

llvm::Value* AstWalker::codeGen_FuncDef(Json::Value json_node)
{ 
    pushScope(ScopeNode::ScopeType::FUNC_SCOPE);

    std::string result_param_name = "result";
    llvm::Function* func          = makeFuncProto(json_node, result_param_name);
    llvm::BasicBlock* entry       = llvm::BasicBlock::Create(currContext, "varDecls", func);
    llvm::BasicBlock* funcBody    = llvm::BasicBlock::Create(currContext, "funcBody");

    scopeHelper->setBlockOnCurrScope(entry);
    Builder.SetInsertPoint(entry);

    {
        int arg_index = 0;
        for(auto argI = func->arg_begin(); arg_index < ((int)func->arg_size()); argI++, arg_index++)
        {
            newVarInScope(argI->getName(), argI);
        }

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
    argsV.push_back(llvm::ConstantInt::get(currContext, llvm::APInt(64, 
                                                                    numVarsInFunc, 
                                                                    false)));
    createCall("gc_push_scope", argsV); 
    Builder.SetInsertPoint(originalBlock);

    popScope();

    return func; 
}

llvm::Value* AstWalker::codeGen_TypedArg(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_ReturnOp(Json::Value json_node)
{ 
    pushScope(ScopeNode::ScopeType::SIMPLE_SCOPE);

    llvm::Value* ret_val = codeGen_initial(json_node);
    //check for return param
    popScope();
    std::vector<llvm::Value*> argsV;
    createCall("gc_pop_scope", argsV); 

    if( scopeHelper->parentFuncReturnsVoid() )
    {
        return Builder.CreateRetVoid();
    }
    else
    {
        return Builder.CreateRet(ret_val); 
    }

}

llvm::Value* AstWalker::AstWalker::codeGen_initial(Json::Value json_node)
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
    TRY_NODE(json_node, TypedArg);
    TRY_NODE(json_node, ReturnOp);

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

llvm::Value* AstWalker::createObject(llvm::Type* obj_type, bool restore_insert_point)
{
    llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
    llvm::BasicBlock& func_block    = originalBlock->getParent()->getEntryBlock();
    
    Builder.SetInsertPoint(&func_block);

    llvm::Value* valuePtr = Builder.CreateAlloca(obj_type);

    if( restore_insert_point )
    {
        Builder.SetInsertPoint(originalBlock);
    }

    return valuePtr;
}

llvm::Value* AstWalker::createConstObject(std::string type_name, llvm::Value* raw_value)
{
    std::string init_func_name = "init_" + type_name;
    std::vector<llvm::Value *> argsV;

    argsV.push_back(raw_value);

    llvm::BasicBlock* originalBlock = Builder.GetInsertBlock();
    llvm::Value* result             = createCall(init_func_name, argsV, false);

    Builder.SetInsertPoint(originalBlock);

    return result;
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
