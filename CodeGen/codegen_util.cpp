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

std::string CodeGenUtil::getTypeStr(llvm::Value* val, bool with_struct_prefix)
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
        //TODO Perhaps find a better way to do this besides substr...
        //This should work assuming all types are pointers to structs
        return result.substr(8, result.length() - 9);
    }
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

std::string CodeGenUtil::typeStrFromStr(std::string type_name)
{
    return "\%struct." + type_name + "*";
}

void CodeGenUtil::assertType(std::string type_name, llvm::Value* val, std::string error_msg)
{
    GEN_ASSERT(CodeGenUtil::typeStrFromStr(type_name) == CodeGenUtil::getTypeStr(val), error_msg);
}

void CodeGenUtil::assertType(llvm::Value* valA, llvm::Value* valB, std::string error_msg)
{
    GEN_ASSERT(CodeGenUtil::getTypeStr(valA, true) == CodeGenUtil::getTypeStr(valB, true), error_msg);
}

uint64_t CodeGenUtil::getPointedToStructSize(llvm::Module * module, llvm::Value* val)
{
        llvm::DataLayout* dl       = new llvm::DataLayout(module);
        llvm::PointerType* ptrType = static_cast<llvm::PointerType*>(val->getType());

        return dl->getTypeAllocSize(ptrType->getElementType());
}

llvm::Type* CodeGenUtil::getVoidStarType(llvm::LLVMContext* currContext)
{
    return llvm::PointerType::get(llvm::Type::getInt8Ty(*currContext), 0);
}

