#include <iostream>
#include <stdexcept>
#include "compile_type.h"

namespace codegen {

int CompileType::VTABLE_OBJECT_INIT_INDEX = 0;
int CompileType::VTABLE_STRUCT_LOCATION = 3;
std::string CompileType::BASE_TYPE_STRUCT_NAME = "Base";

CompileType::CompileType() {}

CompileType::CompileType(const std::string &typeName) : CompileType() {
  this->typeName = typeName;
}

CompileType::CompileType(CommonType commonType) : CompileType() {
  this->typeName = CompileType::getCommonTypeName(commonType);
}

void CompileType::addMethod(const std::string &name, CompileType *funcType) {
  this->methods.push_back(std::make_tuple(name, funcType));
}

void CompileType::addField(const std::string &name, CompileType *fieldType) {
  this->fields.push_back(std::make_tuple(name, fieldType));
}

int CompileType::getMethodIndex(const std::string &name,
                                CompileType *methodType) {
  int i = 0;
  for (auto method : this->methods) {
    // TODO allow for overloading.
    // TODO Check method type for compatibility.
    if (std::get<0>(method) == name) {
      return i;
    }
    i++;
  }
  return -1;
}

int CompileType::getFieldIndex(const std::string &name,
                               CompileType *fieldType) {
  int i = 0;
  for (auto field : this->fields) {
    // TODO Check field type for compatibility.
    if (std::get<0>(field) == name) {
      return i;
    }
    i++;
  }
  return -1;
}

std::string CompileType::getCommonTypeName(CommonType commonType) {
  switch (commonType) {
    case CompileType::CommonType::BASE:
      return "Base";
    case CompileType::CommonType::BOOL:
      return "Bool";
    case CompileType::CommonType::FLOAT:
      return "Float";
    case CompileType::CommonType::FUNCTION:
      return "Function";
    case CompileType::CommonType::INT:
      return "Int";
    case CompileType::CommonType::LIST:
      return "List";
    case CompileType::CommonType::STRING:
      return "String";
    case CompileType::CommonType::VOID:
      return "Void";
    default:
      throw std::runtime_error("Unknown common type!");
  }
}

bool CompileType::isVoidType(const std::string &typeName) {
  return CompileType::getCommonTypeName(CompileType::CommonType::VOID) ==
         typeName;
}

bool CompileType::isVoidType(CompileType *compileType) {
  return CompileType::getCommonTypeName(CompileType::CommonType::VOID) ==
         compileType->getTypeName();
}

bool CompileType::isVoidType(CommonType commonType) {
  return CompileType::CommonType::VOID == commonType;
}

std::string CompileType::getTypeName() { return typeName; }

std::vector<CompileType *> *CompileType::getArgumentsList() {
  return &typeArgumentsList;
}

size_t CompileType::getVtableLength() { return methods.size(); }

void CompileType::insertArgumentsList(CompileType *compileType) {
  typeArgumentsList.push_back(compileType);
}

void CompileType::setArgumentsList(std::vector<CompileType *> *argsList) {
  typeArgumentsList = *argsList;
}

bool CompileType::isEqualToType(CompileType *testType) {
  if (testType == nullptr) {
    return false;
  }
  // Base typename should be the same
  if (this->typeName != testType->typeName) {
    return false;
  }
  std::vector<CompileType *> *argsTypeA = this->getArgumentsList();
  std::vector<CompileType *> *argsTypeB = testType->getArgumentsList();
  if (argsTypeA->size() != argsTypeB->size()) {
    return false;
  }
  for (int i = 0; i < argsTypeA->size(); i++) {
    if (!((*argsTypeA)[i])->isEqualToType((*argsTypeB)[i])) {
      return false;
    }
  }

  return true;
}

bool CompileType::isCompatibleWithType(CompileType *incompleteType) {
  if (incompleteType == nullptr) {
    return true;
  }

  if (incompleteType->typeName == this->typeName) {
    std::vector<CompileType *> *completeTypeArgs = this->getArgumentsList();
    std::vector<CompileType *> *incompleteTypeArgs =
        incompleteType->getArgumentsList();
    size_t completeTypeNumArgs = completeTypeArgs->size();
    size_t incompleteTypeNumArgs = incompleteTypeArgs->size();

    // Function types must have same number of arguments.
    if ((this->typeName == CompileType::getCommonTypeName(
                               CompileType::CommonType::FUNCTION) &&
         (completeTypeNumArgs != incompleteTypeNumArgs))
        // Incomplete type should not be more complete than complete type.
        ||
        (incompleteTypeNumArgs > completeTypeNumArgs)) {
      return false;
    }

    bool result = true;
    for (int i = 0; i < incompleteTypeNumArgs; i++) {
      std::cout << ((*completeTypeArgs)[i])->typeName << std::endl;
      std::cout << ((*incompleteTypeArgs)[i])->typeName << std::endl;
      result &= ((*completeTypeArgs)[i])
                    ->isCompatibleWithType((*incompleteTypeArgs)[i]);
    }

    return result;
  } else {
    return false;
  }
}

CompileType *CompileType::getFunctionReturnType() {
  std::vector<CompileType *> *arguments = this->getArgumentsList();

  // last argument in CompileType is return type
  return (*arguments)[arguments->size() - 1];
}

CompileType *CompileType::getFunctionReturnType(CompileType *functionType) {
  std::vector<CompileType *> *arguments = functionType->getArgumentsList();

  // last argument in CompileType is return type
  return (*arguments)[arguments->size() - 1];
}

bool CompileType::isFunctionType() {
  return this->typeName ==
         CompileType::getCommonTypeName(CompileType::CommonType::FUNCTION);
}

CompileType *CompileType::getParent() { return this->parent; }

void CompileType::setParent(CompileType *parent) { this->parent = parent; }

llvm::FunctionType *CompileType::asRawFunctionType(CompileType *compileType,
                                                   llvm::Type *voidStarTy,
                                                   bool includeThisPointer) {
  int argListSize = compileType->getArgumentsList()->size();
  int paramCount = argListSize;
  // subtract one to avoid counting return type as a parameter.
  paramCount--;
  // Add extra argument for 'this' pointer if desired.
  paramCount += (includeThisPointer ? 1 : 0);
  std::vector<llvm::Type *> argTypes;
  for (int i = 0; i < paramCount; i++) {
    argTypes.push_back(voidStarTy);
  }
  if (!CompileType::isVoidType(
          CompileType::getFunctionReturnType(compileType))) {
    return llvm::FunctionType::get(voidStarTy, argTypes, false);
  }
  return llvm::FunctionType::get(nullptr /* indicates void return */, argTypes,
                                 false);
}

// CompileVal
CompileVal::CompileVal(llvm::Value *rawValue, std::string typeName)
    : compileType(typeName) {
  this->rawValue = rawValue;
}

CompileVal::CompileVal(llvm::Value *rawValue, CompileType *compileType)
    : compileType(*compileType) {
  this->rawValue = rawValue;
}

CompileVal::CompileVal(llvm::Value *rawValue,
                       CompileType::CommonType commonType)
    : compileType(commonType) {
  this->rawValue = rawValue;
}

CompileType *CompileVal::getCompileType() { return &compileType; }

void CompileVal::setCompileType(CompileType *compileType) {
  this->compileType = *compileType;
}

void CompileVal::insertArgumentType(CompileType *compileType) {
  this->compileType.insertArgumentsList(compileType);
}

void CompileVal::setArgumentsList(std::vector<CompileType *> *argsList) {
  this->compileType.setArgumentsList(argsList);
}

llvm::Value *CompileVal::getRawValue() { return rawValue; }

bool CompileVal::typesAreEqual(CompileVal *valB) {
  return this->getCompileType()->isEqualToType(valB->getCompileType());
}

}  // namespace codegen
