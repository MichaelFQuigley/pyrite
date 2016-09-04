#include "codegen_util.h"

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

llvm::Value* CodeGenUtil::getConstInt64(llvm::LLVMContext* currContext, int64_t val, bool is_signed)
{
    return llvm::ConstantInt::get(*currContext, llvm::APInt(64, val, is_signed));
}

bool CodeGenUtil::load_stdlib(std::string stdlib_filename, 
        llvm::Module* currModule, 
        llvm::LLVMContext* currContext)
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

llvm::Value* CodeGenUtil::generateString(llvm::Module* module, std::string str)
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

llvm::Type* CodeGenUtil::getVoidStarType(llvm::LLVMContext* currContext)
{
    return llvm::PointerType::get(llvm::Type::getInt8Ty(*currContext), 0);
}

