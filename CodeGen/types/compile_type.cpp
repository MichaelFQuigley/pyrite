#include <iostream>
#include <stdexcept>
#include "compile_type.h"

namespace codegen {

int CompileType::VTABLE_OBJECT_INIT_INDEX = 0;
int CompileType::VTABLE_STRUCT_LOCATION = 3;
std::string CompileType::BASE_TYPE_STRUCT_NAME = "Base";

CompileType::CompileType() {}
/*
CompileType::CompileType(const CompileType& compileType) {
  // TODO copy generic types.
  this->typeName = compileType.typeName;
  this->parent = compileType.parent;
}
*/
CompileType::CompileType(const std::string &typeName, bool isGeneric)
    : CompileType() {
  this->typeName = typeName;
  this->isGeneric = isGeneric;
}

CompileType::CompileType(const std::string &typeName)
    : CompileType(typeName, false) {}

CompileType::CompileType(CommonType commonType)
    : CompileType(CompileType::getCommonTypeName(commonType)) {}

CompileType *CompileType::makeGenericParam(const std::string &name,
                                           CompileType *parent) {
  CompileType *result = new CompileType(name, true /* isGeneric */);
  result->setParent(parent);
  return result;
}

void CompileType::addMethod(const std::string &name, CompileType *funcType) {
  this->methods.push_back(new TypeMember(name, funcType));
}

void CompileType::addField(const std::string &name, CompileType *fieldType) {
  this->fields.push_back(new TypeMember(name, fieldType));
}

void CompileType::addGeneric(const std::string &genericName,
                             CompileType *parent) {
  this->insertArgumentsList(CompileType::makeGenericParam(genericName, parent));
}

int CompileType::getMethodIndex(const std::string &name,
                                CompileType *methodType) const {
  int i = 0;
  for (auto method : this->methods) {
    // TODO allow for overloading.
    // TODO Check method type for compatibility.
    if (method->getMemberName() == name) {
      return i;
    }
    i++;
  }
  if (this->parent != nullptr) {
    // Check parent for method if it isn't found in current class.
    return getMethodIndex(name, methodType);
  }

  return -1;
}

CompileType *CompileType::getGenericType(const std::string &genericName) const {
  for (auto genericType : this->typeArgumentsList) {
    if (genericType->getTypeName() == genericName) {
      return genericType;
    }
  }

  return nullptr;
}

CompileType const *CompileType::getMethodType(const std::string &name) const {
  int methodIndex = getMethodIndex(name, nullptr);
  if (methodIndex == -1) {
    return nullptr;
  }
  return methods[methodIndex]->getMemberType();
}

int CompileType::getFieldIndex(const std::string &name,
                               CompileType *fieldType) const {
  int i = 0;
  for (auto field : this->fields) {
    // TODO Check field type for compatibility.
    if (field->getMemberName() == name) {
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
    case CompileType::CommonType::INT_RANGE:
      return "IntRange";
    default:
      throw std::runtime_error("Unknown common type!");
  }
}

bool CompileType::isVoidType(const std::string &typeName) {
  return CompileType::getCommonTypeName(CompileType::CommonType::VOID) ==
         typeName;
}

bool CompileType::isVoidType(CompileType const *compileType) {
  return CompileType::getCommonTypeName(CompileType::CommonType::VOID) ==
         compileType->getTypeName();
}

bool CompileType::isVoidType(CommonType commonType) {
  return CompileType::CommonType::VOID == commonType;
}

std::string CompileType::getTypeName() const { return typeName; }

std::vector<CompileType *> const *CompileType::getArgumentsList() const {
  return &typeArgumentsList;
}

size_t CompileType::getVtableLength() { return methods.size(); }

void CompileType::insertArgumentsList(CompileType *compileType) {
  typeArgumentsList.push_back(compileType);
}

void CompileType::setArgumentsList(std::vector<CompileType *> *argsList) {
  typeArgumentsList = *argsList;
}

bool CompileType::isEqualToType(CompileType const *testType) const {
  if (testType == nullptr) {
    return false;
  }
  // Base typename should be the same
  if (this->typeName != testType->typeName) {
    return false;
  }
  std::vector<CompileType *> const *argsTypeA = this->getArgumentsList();
  std::vector<CompileType *> const *argsTypeB = testType->getArgumentsList();
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

bool CompileType::isTypeOrSubtype(CompileType const *typeA,
                                  CompileType const *typeB) {
  return (typeA != nullptr) && (typeA->isEqualToType(typeB) ||
                                isTypeOrSubtype(typeA->getParent(), typeB));
}

bool CompileType::typeImplementsGeneric(CompileType const *typeA,
                                        CompileType const *typeB) {
  if (typeA->isGenericType()) {
    throw std::runtime_error("First argument is a generic type!");
  }
  if (!typeB->isGenericType()) {
    throw std::runtime_error("Second argument is not a generic type!");
  }
  return CompileType::isTypeOrSubtype(typeA, typeB->getParent());
}

bool CompileType::isCompatibleWithType(CompileType *incompleteType) {
  if (incompleteType == nullptr) {
    return true;
  }

  if (incompleteType->typeName == this->typeName) {
    std::vector<CompileType *> const *completeTypeArgs =
        this->getArgumentsList();
    std::vector<CompileType *> const *incompleteTypeArgs =
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
      result &= ((*completeTypeArgs)[i])
                    ->isCompatibleWithType((*incompleteTypeArgs)[i]);
    }

    return result;
  } else {
    return false;
  }
}

CompileType const *CompileType::getFunctionReturnType() const {
  std::vector<CompileType *> const *arguments = this->getArgumentsList();

  // last argument in CompileType is return type
  return (*arguments)[arguments->size() - 1];
}

CompileType const *CompileType::getFunctionReturnType(
    CompileType const *functionType) {
  std::vector<CompileType *> const *arguments =
      functionType->getArgumentsList();

  // last argument in CompileType is return type
  return (*arguments)[arguments->size() - 1];
}

bool CompileType::isFunctionType() const {
  return this->typeName ==
         CompileType::getCommonTypeName(CompileType::CommonType::FUNCTION);
}

bool CompileType::isGenericType() const { return isGeneric; }

CompileType const *CompileType::getParent() const { return this->parent; }

void CompileType::setParent(CompileType *parent) { this->parent = parent; }

llvm::FunctionType *CompileType::asRawFunctionType(
    CompileType const *compileType, llvm::Type *voidStarTy,
    llvm::LLVMContext &currContext, bool includeThisPointer) {
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
  return llvm::FunctionType::get(llvm::Type::getVoidTy(currContext), argTypes,
                                 false);
}

// CompileVal
CompileVal::CompileVal(llvm::Value *rawValue, std::string typeName)
    : compileType(typeName) {
  this->rawValue = rawValue;
}

CompileVal::CompileVal(llvm::Value *rawValue, CompileType const *compileType)
    : compileType(*compileType) {
  this->rawValue = rawValue;
}

CompileVal::CompileVal(llvm::Value *rawValue,
                       CompileType::CommonType commonType)
    : compileType(commonType) {
  this->rawValue = rawValue;
}

CompileType *CompileVal::getCompileType() { return &compileType; }

std::string CompileType::to_string(const CompileType &compileType) {
  return compileType.to_string();
}

std::string CompileType::to_string() const {
  // TODO maybe make this more efficient.
  std::string result = this->getTypeName();
  if (!this->typeArgumentsList.empty()) {
    result += "<";
    for (CompileType *typeArg : this->typeArgumentsList) {
      result += CompileType::to_string(*typeArg);
    }
    result += ">";
  }
  if (!this->methods.empty()) {
    result += "{methods: ";
    for (auto method : this->methods) {
      result += method->getMemberName() + "(...), ";
    }
    result += "}";
  }
  return result;
}

std::string CompileType::to_string(CompileType *compileType) {
  return CompileType::to_string(*compileType);
}

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

TypeMember::TypeMember(const std::string &memberName,
                       CompileType const *compileType) {
  this->memberName = memberName;
  this->memberType = compileType;
  this->flags = FlagBit::NONE;
}

// TODO add constructor which sets flags.

std::string TypeMember::getMemberName() const { return memberName; }

CompileType const *TypeMember::getMemberType() const { return memberType; }

void TypeMember::setMemberType(CompileType const *compileType) {
  this->memberType = compileType;
}
}  // namespace codegen
