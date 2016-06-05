#include <iostream>
#include <cstdlib>
#include "ast_walker.h"
#include "codegen_util.h"

using namespace std;

AstWalker::AstWalker(std::string filename, std::string stdlib_filename) : Builder(currContext)
{

    currModule  = llvm::make_unique<llvm::Module>(filename, currContext);
    scopeHelper = new ScopeHelper();

    if( !load_stdlib(stdlib_filename) )
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

llvm::Type* AstWalker::getTypeFromStr(std::string typeName)
{
    if( typeName == "Void" )
    {
        return llvm::Type::getVoidTy(currContext);
    }

    llvm::StructType* type = currModule->getTypeByName("struct." + typeName);
    GEN_ASSERT(type != nullptr, "Unknown type.");
    return llvm::PointerType::getUnqual(type);
}

llvm::Function* AstWalker::makeFuncProto(Json::Value json_node)
{
    std::vector<llvm::Type*> argsV;
    Json::Value header_node = json_node["header"]["FuncProto"];
    Json::Value retTypeNode = header_node["ret_type"];
     GEN_ASSERT(retTypeNode["simple"], 
             "Simples are the only types supported at the moment");
    std::string retTypeName = retTypeNode["simple"].asString();
    llvm::Type* retType     = getTypeFromStr(retTypeName);
    for( const Json::Value& val : header_node["args"] )
    {
        Json::Value typedArg = val["TypedArg"];
        Json::Value argType  = typedArg["type"];

        GEN_ASSERT(argType["simple"], 
                "Simples are the only types supported at the moment");

        std::string argTypeName = argType["simple"].asString();
        
        argsV.push_back(getTypeFromStr(argTypeName));
    }

    std::string funcName          = json_node["name"].asString();
    llvm::FunctionType* funcProto = llvm::FunctionType::get(retType, argsV, false);    
    llvm::Function* func          = llvm::Function::Create(funcProto, 
                                        llvm::Function::ExternalLinkage, 
                                        funcName,
                                        currModule.get());


    unsigned arg_index = 0;
    for( auto &argI : func->args() )
    {
        argI.setName(header_node["args"][arg_index++]["TypedArg"]["name"].asString());
    }

    return func; 
}

llvm::Value* AstWalker::codeGen_ExprOp(Json::Value json_node){ return codeGen_initial(json_node); }

llvm::Value* AstWalker::codeGen_VarDef(Json::Value json_node){ 
    std::string var_name  = json_node["var"]["TypedArg"]["name"].asString();
    llvm::Value* rhs_expr = codeGen_initial(json_node["expr"]); 
    auto allocaRes = Builder.CreateAlloca(rhs_expr->getType());
    Builder.CreateStore(rhs_expr, allocaRes);
    scopeHelper->setNamedVal(var_name, allocaRes, true);
    return nullptr; 
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
            return Builder.CreateStore(codeGen_initial(json_node["rhs"]), var_val);
        }
        else
        {
            throw;
            return nullptr;
        }
    }
    else if( op == "+" )  { op_func_prefix = "add_";}
    else if( op == "-" )  { op_func_prefix = "sub_";}
    else if( op == "*" )  { op_func_prefix = "mul_";}
    else if( op == "/" )  { op_func_prefix = "div_";}
    else if( op == "%" )  { op_func_prefix = "mod_";}
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
    //turn struct type into string...
    std::string type_str = getTypeStr(lhs);
    std::string lhs_type_str = type_str.substr(8, type_str.length() - 9);
    std::string op_funcname = op_func_prefix + lhs_type_str; 

    auto op_func = currModule->getFunction(op_funcname);
    return Builder.CreateCall(op_func, argsV, op_funcname);
}

std::string AstWalker::getTypeStr(llvm::Value* val, bool with_struct_prefix)
{

    std::string type_str = "";
    llvm::raw_string_ostream rso(type_str);
    auto val_type = val->getType();
    val_type->print(rso);
    std::string result = rso.str();

    if( with_struct_prefix )
    {
        return result;
    }
    else
    {
        return result.substr(8, result.length() - 9);
    }
}

std::string AstWalker::typeStrFromStr(std::string type_name)
{
    return "\%struct." + type_name + "*";
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
                return createValueObject("Float", 
                                  llvm::ConstantFP::get(currContext, 
                                      llvm::APFloat(double_val)));
                break;
            case 'i':
                int_val = stol(lit_val);
                return createValueObject("Int", 
                                  llvm::ConstantInt::get(currContext, 
                                      llvm::APInt(64, int_val, true)));
                break;
            case 's':
                 return createValueObject("String", 
                                    generateString(currModule.get(), lit_val));
            case 'b':
                int_val = (lit_val == "true") ? 1 : 0;
                return createValueObject("Bool", 
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
    else
    {
        cout << "Unimplemented atom value." << endl;
    }
    return nullptr;
}

llvm::BasicBlock* AstWalker::makeBasicBlock(std::string name)
{
    return llvm::BasicBlock::Create(currContext, 
                                    name, 
                                    Builder.GetInsertBlock()->getParent());
}

llvm::Value* AstWalker::createCall(std::string func_name, 
        std::vector<llvm::Value*> argsV, 
        bool raise_fail_exception, 
        std::string error_msg)
{
    std::string full_func_name = func_name;
    //Upper case function call is by convention a constructor call
    if( isupper(func_name[0]) )
    {
        for(auto &argI : argsV)
        {
            full_func_name += "_" + getTypeStr(argI, false);
        }
    }
    auto func = currModule->getFunction(full_func_name);

    if( func == nullptr )
    {
        if( raise_fail_exception ) 
        {  
            cout << error_msg << full_func_name << endl;
            throw;
        }
        return nullptr;
    }

    return Builder.CreateCall(func, argsV);
}

llvm::Value* AstWalker::codeGen_LoopOp(Json::Value json_node){ 
    std::vector<llvm::Value*> argsV;
    llvm::Function *currFunction = Builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* loopTop    = makeBasicBlock("loopTop");
    llvm::BasicBlock* loopBody   = llvm::BasicBlock::Create(currContext, "loopBody");
    llvm::BasicBlock* loopBottom = llvm::BasicBlock::Create(currContext, "loopBottom");

    int args_length = json_node["args"].size();

    scopeHelper->pushScope();

    //codegen initial statement
    if( args_length == 3 )
        codeGen_initial(json_node["args"][0]); 

    Builder.CreateBr(loopTop);
    Builder.SetInsertPoint(loopTop);

    if( args_length == 0 )
    {
        codeGen_initial(json_node["body"]);
    }

    if( args_length > 0 )
    {   
        int args_index        = (args_length == 1) ? 0 : 1;
        llvm::Value* test_val = codeGen_initial(json_node["args"][args_index]); 

        assertType("Bool", test_val, "Single arg in loop should be type Bool.");

        argsV.push_back(test_val);
        llvm::Value* raw_bool = createCall("rawVal_Bool", argsV);
        Builder.CreateCondBr(raw_bool, loopBody, loopBottom);
        currFunction->getBasicBlockList().push_back(loopBody);
        Builder.SetInsertPoint(loopBody);
        codeGen_initial(json_node["body"]);

        if( args_length == 3 )
        {
            codeGen_initial(json_node["args"][2]);
        }
    }

    Builder.CreateBr(loopTop);
    currFunction->getBasicBlockList().push_back(loopBottom);
    Builder.SetInsertPoint(loopBottom);

    scopeHelper->popScope();

    return nullptr;
}

llvm::Value* AstWalker::codeGen_IfOp(Json::Value json_node)
{
    llvm::Value* test = codeGen_initial(json_node["test"]);
    llvm::Function *currFunction = Builder.GetInsertBlock()->getParent();
    std::vector<llvm::Value*> argsV;
    argsV.push_back(test);
    llvm::Constant* one     = llvm::ConstantInt::get(currContext, llvm::APInt(1, 1, false));
    llvm::Value* raw_bool   = createCall("rawVal_Bool", argsV);
    llvm::Value* testResult = Builder.CreateICmpEQ(raw_bool, one);
    
    llvm::BasicBlock* ifTrue = makeBasicBlock("ifTrue");
    llvm::BasicBlock* endIf;

    llvm::Value* result = nullptr;
    //check to see if there is an else block
    if( json_node["bodies"].size() == 1 )
    {
        scopeHelper->pushScope();
        endIf = llvm::BasicBlock::Create(currContext, "endif");
        Builder.CreateCondBr(testResult, ifTrue, endIf);
        Builder.SetInsertPoint(ifTrue);
        codeGen_initial(json_node["bodies"][0]);
        //currFunction->getBasicBlockList().push_back(endIf);
        Builder.CreateBr(endIf);
        Builder.SetInsertPoint(endIf); 
        scopeHelper->popScope();
    }
    //else block (size == 2)
    else
    {
        llvm::BasicBlock* ifFalse = llvm::BasicBlock::Create(currContext, "ifFalse");
        endIf = llvm::BasicBlock::Create(currContext, "endif");
        Builder.CreateCondBr(testResult, ifTrue, ifFalse);
        //if true codegen
        scopeHelper->pushScope();
        Builder.SetInsertPoint(ifTrue);
        llvm::Value* ifTrueLastStmt = codeGen_initial(json_node["bodies"][0]);
        scopeHelper->popScope();
        //if false codegen
        scopeHelper->pushScope();
        Builder.CreateBr(endIf);
        ifTrue = Builder.GetInsertBlock();
        currFunction->getBasicBlockList().push_back(ifFalse);
        Builder.SetInsertPoint(ifFalse);
        llvm::Value* ifFalseLastStmt = codeGen_initial(json_node["bodies"][1]);
        Builder.CreateBr(endIf);
        ifFalse = Builder.GetInsertBlock();
        currFunction->getBasicBlockList().push_back(endIf);
        Builder.SetInsertPoint(endIf); 
        scopeHelper->popScope();
        //phi node
        GEN_ASSERT(getTypeStr(ifTrueLastStmt) == getTypeStr(ifFalseLastStmt), 
                "Types at end of if-else must match.");
        llvm::PHINode * phi = Builder.CreatePHI(ifTrueLastStmt->getType(), 2, "ifPhi");
        phi->addIncoming(ifTrueLastStmt, ifTrue);
        phi->addIncoming(ifFalseLastStmt, ifFalse);
        result = phi;
    }

    return result;
}

void AstWalker::assertType(std::string type_name, llvm::Value* val, std::string error_msg)
{
    GEN_ASSERT(typeStrFromStr(type_name) == getTypeStr(val), error_msg);
}

llvm::Value* AstWalker::codeGen_FuncDef(Json::Value json_node)
{ 
    scopeHelper->pushScope();

    llvm::Function* func = makeFuncProto(json_node);
    
    for( auto &argI : func->args() )
    {
        scopeHelper->setNamedVal(argI.getName(), &argI, true);
    }


    llvm::BasicBlock* entry = llvm::BasicBlock::Create(currContext, "entry", func);
    Builder.SetInsertPoint(entry);
    codeGen_initial(json_node["stmts"]);

    scopeHelper->popScope();

    return func; 
}

llvm::Value* AstWalker::codeGen_TypedArg(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_ReturnOp(Json::Value json_node){ 
    llvm::Value* ret_val = codeGen_initial(json_node);

    if( ret_val == nullptr )
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
    TRY_NODE(json_node, LoopOp);
    TRY_NODE(json_node, IfOp);
    TRY_NODE(json_node, FuncDef);
    TRY_NODE(json_node, TypedArg);
    TRY_NODE(json_node, ReturnOp);

    //If none of the TRY_NODE blocks returned anything, then we have an unimplemented ast node.
    GEN_ASSERT(false, "Unimplemented node type in code generator");

    return nullptr;
}

void AstWalker::codeGen_top(Json::Value json_node)
{
    codeGen_initial(json_node);
}

Json::Value AstWalker::generateFromJson(std::string json_string)
{
    Json::Reader reader;
    Json::Value  json_root;
    reader.parse(json_string, json_root);
    return json_root;
}

llvm::Value* AstWalker::createValueObject(std::string type_name, llvm::Value* value)
{
    std::string init_func_name = "init_" + type_name;
    std::vector<llvm::Value *> argsV;
    argsV.push_back(value);

    return createCall(init_func_name, argsV);
}

llvm::Value* AstWalker::generateString(llvm::Module* module, std::string str)
{
    llvm::ArrayType* ArrayTy = llvm::ArrayType::get(llvm::IntegerType::get(module->getContext(), 8), 
                                                     str.length() + 1);
    llvm::GlobalVariable* str_gvar = new llvm::GlobalVariable(
            *module,
            ArrayTy,
            true,
            llvm::GlobalValue::PrivateLinkage,
            nullptr,
            ".str");

    llvm::Constant* str_arr  = llvm::ConstantDataArray::getString(module->getContext(), str);
    llvm::ConstantInt* const_int64 = llvm::ConstantInt::get(module->getContext(), 
                                                      llvm::APInt(64, 0, true));
    std::vector<llvm::Constant*> indices;
    indices.push_back(const_int64);
    indices.push_back(const_int64);
    llvm::Constant* result = llvm::ConstantExpr::getGetElementPtr(str_gvar, indices);
    str_gvar->setInitializer(str_arr);
    return result;
}

bool AstWalker::load_stdlib(std::string stdlib_filename)
{
    llvm::LLVMContext &stdlib_context = currContext;
    llvm::SMDiagnostic Err;
    std::unique_ptr<llvm::Module> stdlib_mod = llvm::parseIRFile(stdlib_filename, 
                                                                    Err, 
                                                                    stdlib_context);
    if( stdlib_mod == nullptr )
    {
        cout << "Error loading stdlib" << endl;
        return false;
    }
    //Pull in functions
    llvm::Module::FunctionListType &stdlib_functions = stdlib_mod->getFunctionList();
    for( auto i = stdlib_functions.begin(); i != stdlib_functions.end(); i++ )
    {
        llvm::Function &func  = *i;
        std::string func_name = func.getName();
        llvm::Function::Create(func.getFunctionType(), 
                llvm::Function::ExternalLinkage, 
                func_name, 
                currModule.get());
    }
    /*Note: struct types are already begin pulled in automoatically when function are since 
     * the functions use the structs. They will have to be pulled in if this is ever not the case.*/
    return true;
}

void AstWalker::dumpIR()
{
    currModule->dump();
}

void AstWalker::writeToFile(std::string filename)
{
    std::error_code errs;
    //TODO: make raw_fd_ostream decl more portable
    llvm::raw_fd_ostream file(llvm::StringRef(filename), errs, (llvm::sys::fs::OpenFlags)8);
    GEN_ASSERT(errs.value() == 0, "Error writing file.");
    llvm::WriteBitcodeToFile(currModule.get(), file);
}

int main()
{
    AstWalker* ast = new AstWalker("teeeest", "../CodeGen/stdlib/stdlib.ll");
    std::string json_string;

    if( std::cin ) 
    {
        getline(std::cin, json_string);
        ast->codeGen_top(ast->generateFromJson(json_string));
        ast->dumpIR();
        ast->writeToFile("../Lexers/test.bc");
    }
}
