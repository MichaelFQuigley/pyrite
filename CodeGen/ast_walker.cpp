#include <iostream>
#include <cstdlib>
#include "ast_walker.h"

using namespace std;

AstWalker::AstWalker(std::string filename, std::string stdlib_filename) : Builder(currContext)
{

    currModule = llvm::make_unique<llvm::Module>(filename, currContext);
    llvm::BasicBlock *globalBlock = llvm::BasicBlock::Create(currContext);

    Builder.SetInsertPoint(globalBlock);
    
    if( !load_stdlib(stdlib_filename) )
    {
        return;
    }
    llvm::FunctionType *main_functype = llvm::FunctionType::get(llvm::Type::getVoidTy(currContext), 
                                                        false);
    llvm::Function* main_func = llvm::Function::Create(main_functype, 
                                                        llvm::Function::ExternalLinkage,
                                                        "main", 
                                                        currModule.get());
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(currContext, "mainBlock", main_func);
    Builder.SetInsertPoint(BB);
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

llvm::Value* AstWalker::codeGen_FuncProto(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_ExprOp(Json::Value json_node){ return codeGen_initial(json_node); }

llvm::Value* AstWalker::codeGen_VarDef(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_BinOp(Json::Value json_node){
    std::string op = json_node["op"].asString();
    std::string op_func_prefix = "";
    
    if(op == "+")      { op_func_prefix = "add_";}
    else if(op == "-") { op_func_prefix = "sub_";}
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
    std::string type_str = "";
    llvm::raw_string_ostream rso(type_str);
    auto lhs_type = lhs->getType();
    lhs_type->print(rso);
    std::string lhs_type_str = rso.str().substr(8, rso.str().length() - 9);
    std::string op_funcname = op_func_prefix + lhs_type_str; 

    auto op_func = currModule->getFunction(op_funcname);
    return Builder.CreateCall(op_func, argsV, op_funcname);
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
        switch(lit_type)
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
            default:
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

        auto func = currModule->getFunction(func_name);
        return Builder.CreateCall(func, argsV);
    }
    else if( json_node_has(json_node, "Variable", &val_node) ) {}
    else
    {
        cout << "Unimplemented atom value." << endl;
    }
    return nullptr;
}

llvm::Value* AstWalker::codeGen_LoopOp(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_IfOp(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_FuncDef(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::codeGen_TypedArg(Json::Value json_node){ return nullptr; }

llvm::Value* AstWalker::AstWalker::codeGen_initial(Json::Value json_node)
{
    TRY_NODE(json_node, StmtsOp);
    TRY_NODE(json_node, FuncProto);
    TRY_NODE(json_node, ExprOp);
    TRY_NODE(json_node, VarDef);
    TRY_NODE(json_node, BinOp);
    TRY_NODE(json_node, AtomOp);
    TRY_NODE(json_node, LoopOp);
    TRY_NODE(json_node, IfOp);
    TRY_NODE(json_node, FuncDef);
    TRY_NODE(json_node, TypedArg);

    //If none of the TRY_NODE blocks returned anything, then we have an unimplemented ast node.
    std::cout << "Unimplemented node type in code generator" << std::endl;

    return nullptr;
}

void AstWalker::codeGen_top(Json::Value json_node)
{
    codeGen_initial(json_node);
    Builder.CreateRetVoid();
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
    auto func = currModule->getFunction(init_func_name);

    if( func == nullptr )
    {
        cout << "Error getting function for type " << type_name << endl;
        return nullptr;
    }

    return Builder.CreateCall(func, argsV, init_func_name);
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
    for(auto i = stdlib_functions.begin(); i != stdlib_functions.end(); i++)
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

std::string AstWalker::dumpIR()
{
    currModule->dump();
    return "";
}

void AstWalker::writeToFile(std::string filename)
{
    std::error_code errs;
    llvm::raw_fd_ostream file(llvm::StringRef(filename), errs, llvm::sys::fs::OpenFlags::F_RW);


    llvm::WriteBitcodeToFile(currModule.get(), file);
}

int main()
{
    AstWalker* ast = new AstWalker("teeeest", "../CodeGen/stdlib/stdlib.ll");
    std::string json_string;

    if(std::cin) 
    {
        getline(std::cin, json_string);
        ast->codeGen_top(ast->generateFromJson(json_string));
        ast->dumpIR();
    }
}
