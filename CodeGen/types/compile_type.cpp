#include <iostream>
#include <stdexcept>
#include "compile_type.h"

namespace codegen {

int CompileType::VTABLE_OBJECT_INIT_INDEX = 0;
int CompileType::VTABLE_STRUCT_LOCATION = 3;
std::string CompileType::BASE_TYPE_STRUCT_NAME = "Base";

CompileType::CompileType() {}

CompileType::CompileType(const CompileType &compileType) {
  this->typeName = compileType.typeName;
  this->parent = compileType.parent;
  this->isGeneric = compileType.isGeneric;
  this->typeArgumentsList = compileType.typeArgumentsList;
  this->fields = compileType.fields;
  this->methods = compileType.methods;
}

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

CompileType *CompileType::implementGenericHelper(
    const std::string &genericName, CompileType const *concreteType,
    CompileType const *containingClass,
    std::map<std::string, CompileType *> *nameToImplementedType) {
  // If there is no generic type to implement, just return original containing
  // class
  // since nothing needs to be copied.
  if (!CompileType::hasGeneric(containingClass, genericName)) {
    // TODO use something better than const cast.
    return const_cast<CompileType *>(containingClass);
  }
  // Check map to see if we have already made this type concrete.
  if (containingClass->getTypeName() !=
          CompileType::getCommonTypeName(CompileType::CommonType::FUNCTION) &&
      nameToImplementedType->find(containingClass->getTypeName()) !=
          nameToImplementedType->end()) {
    return (*nameToImplementedType)[containingClass->getTypeName()];
  }
  // Copy containing class into class that will be returned.
  CompileType *resultType = new CompileType(*containingClass);
  (*nameToImplementedType)[resultType->getTypeName()] = resultType;

  // Find generics to replace with concrete type in type arguments.
  for (int i = 0; i < resultType->typeArgumentsList.size(); i++) {
    CompileType *testType = resultType->typeArgumentsList[i];
    if (testType->getTypeName() == genericName) {
      // Concrete type must be able to implement generic parameter.
      if (!CompileType::typeImplementsGeneric(concreteType, testType)) {
        throw std::runtime_error("Generic parameter " + genericName +
                                 " cannot be" + " implemented by type: " +
                                 concreteType->to_string());
      }
      resultType->typeArgumentsList[i] =
          const_cast<CompileType *>(concreteType);
    } else if (CompileType::hasGeneric(testType, genericName)) {
      if (testType->getTypeName() == containingClass->getTypeName()) {
        resultType->typeArgumentsList[i] =
            const_cast<CompileType *>(containingClass);
      } else {
        resultType->typeArgumentsList[i] = CompileType::implementGenericHelper(
            genericName, concreteType, testType, nameToImplementedType);
      }
    }
  }
  // Find generics to replace in methods list.
  for (int i = 0; i < resultType->methods.size(); i++) {
    TypeMember *testMember = resultType->methods[i];
    if (CompileType::hasGeneric(testMember->getMemberType(), genericName)) {
      resultType->methods[i] = new TypeMember(
          testMember->getMemberName(),
          CompileType::implementGenericHelper(genericName, concreteType,
                                              testMember->getMemberType(),
                                              nameToImplementedType));
    }
  }
  // Find generics to replace in fields list.
  for (int i = 0; i < resultType->fields.size(); i++) {
    TypeMember *testMember = resultType->fields[i];
    if (CompileType::hasGeneric(testMember->getMemberType(), genericName)) {
      resultType->fields[i] = new TypeMember(
          testMember->getMemberName(),
          CompileType::implementGenericHelper(genericName, concreteType,
                                              testMember->getMemberType(),
                                              nameToImplementedType));
    }
  }
  return resultType;
}

CompileType *CompileType::implementGeneric(const std::string &genericName,
                                           CompileType const *concreteType,
                                           CompileType const *containingClass) {
  std::map<std::string, CompileType *> nameToImplementedType;
  return implementGenericHelper(genericName, concreteType, containingClass,
                                &nameToImplementedType);
}

void CompileType::addGeneric(const std::string &genericName,
                             CompileType *parent) {
  this->insertArgumentsList(CompileType::makeGenericParam(genericName, parent));
}

bool CompileType::hasGeneric(CompileType const *containingClass,
                             const std::string &genericName) {
  for (auto typeArg : containingClass->typeArgumentsList) {
    if (typeArg->getTypeName() == genericName && typeArg->isGenericType()) {
      return true;
    }
    if (hasGeneric(typeArg, genericName)) {
      return true;
    }
  }

  return false;
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

bool CompileType::isVoidType() const { return CompileType::isVoidType(this); }

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

bool CompileType::isEqualToType(CompileType const *testType,
                                bool checkGenericArgs) const {
  if (testType == nullptr) {
    return false;
  }
  // Base typename should be the same
  if (this->typeName != testType->typeName) {
    return false;
  }
  if (checkGenericArgs) {
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
  }

  return true;
}

bool CompileType::lhsTypeCanReplaceRhsType(CompileType const *lhsType,
                                           CompileType const *rhsType) {
  std::vector<CompileType *> const *lhsArgs = lhsType->getArgumentsList();
  std::vector<CompileType *> const *rhsArgs = rhsType->getArgumentsList();
  if (lhsArgs->size() !=
      rhsArgs->size()) {  // Must have same number of generic parameters.
    return false;
  }
  for (int i = 0; i < lhsArgs->size(); i++) {
    CompileType *lhsArg = (*lhsArgs)[i];
    CompileType *rhsArg = (*rhsArgs)[i];
    if (rhsArg->isGenericType() && lhsArg->isGenericType() &&
        !rhsArg->isEqualToType(
            lhsArg)) {  // If both types are generic, they must be equal.
      return false;
    } else if (rhsArg->isGenericType()) {
      // If just rhs is generic, lhs must implement it.
      if (!CompileType::typeImplementsGeneric(lhsArg, rhsArg)) {
        return false;
      }
    } else if (lhsArg->isGenericType()) {
      // Rhs must also be generic if lhs is generic, so just return false.
      return false;
    } else if (!CompileType::lhsTypeCanReplaceRhsType(lhsArg, rhsArg)) {
      // Else, neither type is generic, so check to see if
      return false;
    }
  }
  return CompileType::isTypeOrSubtype(rhsType, lhsType,
                                      false /* checkGenericArgs */);
}

bool CompileType::isTypeOrSubtype(CompileType const *typeA,
                                  CompileType const *typeB,
                                  bool checkGenericArgs) {
  return (typeA != nullptr) &&
         (typeA->isEqualToType(typeB, checkGenericArgs) ||
          isTypeOrSubtype(typeA->getParent(), typeB, checkGenericArgs));
}

bool CompileType::typeImplementsGeneric(CompileType const *typeA,
                                        CompileType const *typeB) {
  if (typeA->isGenericType()) {
    return false;
  }
  if (!typeB->isGenericType()) {
    return false;
  }
  return CompileType::isTypeOrSubtype(typeA, typeB->getParent());
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
  // subtract one to avoid counting return type as a parameter.
  int paramCount = argListSize - 1;
  std::vector<llvm::Type *> argTypes;
  // Add extra argument for 'this' pointer if desired.
  if (includeThisPointer) {
    argTypes.push_back(voidStarTy);
  }
  for (int i = 0; i < paramCount; i++) {
    argTypes.push_back(voidStarTy);
  }
  if (!CompileType::getFunctionReturnType(compileType)->isVoidType()) {
    return llvm::FunctionType::get(voidStarTy, argTypes, false);
  }
  return llvm::FunctionType::get(llvm::Type::getVoidTy(currContext), argTypes,
                                 false);
}

// CompileVal
CompileVal::CompileVal(llvm::Value *rawValue, std::string typeName,
                       bool isBoxed)
    : compileType(typeName) {
  this->rawValue = rawValue;
  this->isBoxed = isBoxed;
}

CompileVal::CompileVal(llvm::Value *rawValue, CompileType const *compileType,
                       bool isBoxed)
    : compileType(*compileType) {
  this->rawValue = rawValue;
  this->isBoxed = isBoxed;
}

CompileVal::CompileVal(llvm::Value *rawValue,
                       CompileType::CommonType commonType, bool isBoxed)
    : compileType(commonType) {
  this->rawValue = rawValue;
  this->isBoxed = isBoxed;
}

CompileType *CompileVal::getCompileType() { return &compileType; }

std::string CompileType::getLongTypeName() const {
  std::string result = this->getTypeName();
  if (!this->typeArgumentsList.empty()) {
    result += "<";
    for (CompileType *typeArg : this->typeArgumentsList) {
      result += typeArg->getLongTypeName() + ", ";
    }
    result += ">";
  }
  return result;
}

std::string CompileType::to_string(const CompileType &compileType) {
  return compileType.to_string();
}

std::string CompileType::to_string() const {
  // TODO maybe make this more efficient.
  std::string result = this->getLongTypeName();

  if (!this->methods.empty()) {
    result += "{methods: ";
    for (auto method : this->methods) {
      CompileType const *methodProto = method->getMemberType();
      result += method->getMemberName() + "(";
      for (auto typeArg : methodProto->typeArgumentsList) {
        result += typeArg->getTypeName() + ", ";
      }
      result += "), ";
    }
    result += "}";
  }
  return result;
}

std::string CompileType::to_string(CompileType *compileType) {
  return CompileType::to_string(*compileType);
}

void CompileVal::setCompileType(CompileType const *compileType) {
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

bool CompileVal::getIsBoxed() const { return isBoxed; }

CompileVal *CompileVal::box(llvm::Value *boxedValue) {
  rawValue = boxedValue;
  isBoxed = true;
  return this;
}

CompileVal *CompileVal::unbox(llvm::Value *unboxedValue) {
  rawValue = unboxedValue;
  isBoxed = false;
  return this;
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
