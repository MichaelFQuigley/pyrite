#include "codegen_util.h"

CodeGenUtil::CodeGenUtil(llvm::Module* currModule, llvm::LLVMContext* currContext)
{
    this->currModule  = currModule;
    this->currContext = currContext;
    hidden_var_count  = 0;
}

std::string CodeGenUtil::getNewHiddenVarName()
{
    return "$temp_" + std::to_string(hidden_var_count++);
}

void CodeGenUtil::writeToFile(std::string filename, llvm::Module* currModule)
{
    std::error_code errs;
    //TODO make raw_fd_ostream decl more portable
    llvm::raw_fd_ostream file(llvm::StringRef(filename), errs, (llvm::sys::fs::OpenFlags)8);
    GEN_ASSERT(errs.value() == 0, "Error writing file.");
    llvm::WriteBitcodeToFile(currModule, file);
}

void CodeGenUtil::dumpIR(llvm::Module* currModule)
{
    currModule->dump();
}

llvm::Value* CodeGenUtil::getConstInt64(int64_t val, bool is_signed)
{
    return llvm::ConstantInt::get(*currContext, llvm::APInt(64, val, is_signed));
}

bool CodeGenUtil::load_stdlib(std::string stdlib_filename)
{
    llvm::LLVMContext &stdlib_context = *currContext;
    llvm::SMDiagnostic Err;
    std::unique_ptr<llvm::Module> stdlib_mod = llvm::parseIRFile(stdlib_filename, 
                                                                    Err, 
                                                                    stdlib_context);
    if( stdlib_mod == nullptr )
    {
        std::cout << "Error loading stdlib" << std::endl;
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
                currModule);
    }
    /*NOTE: struct types are already being pulled in automatically when function are since 
     * the functions use the structs. They will have to be pulled in if this is ever not the case.*/
    return true;
}

llvm::Value* CodeGenUtil::generateString(std::string str)
{
    llvm::ArrayType* ArrayTy = llvm::ArrayType::get(llvm::IntegerType::get(*currContext, 8), 
                                                     str.length() + 1);
    llvm::GlobalVariable* str_gvar = new llvm::GlobalVariable(
            *currModule,
            ArrayTy,
            true,
            llvm::GlobalValue::PrivateLinkage,
            nullptr,
            ".str");

    llvm::Constant* str_arr  = llvm::ConstantDataArray::getString(*currContext, str);
    llvm::ConstantInt* const_int64 = llvm::ConstantInt::get(*currContext, 
                                                      llvm::APInt(64, 0, true));
    std::vector<llvm::Constant*> indices;
    indices.push_back(const_int64);
    indices.push_back(const_int64);
    llvm::Constant* result = llvm::ConstantExpr::getGetElementPtr(str_gvar, indices);
    str_gvar->setInitializer(str_arr);

    return result;
}
