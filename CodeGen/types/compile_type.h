#ifndef COMPILE_TYPE_H
#define COMPILE_TYPE_H

/*
 * compile_type manages type information. The CompileType class deals with
 * storing
 * and enforcing objects at compile time. The CompileVal class deals with
 * specific
 * instances of a type.
 */

#include <map>
#include <vector>
#include <string>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace codegen {

class TypeMember;

class CompileType {
 private:
  std::string typeName;
  CompileType* parent;
  bool isGeneric;
  // genericMappings contains mappings from a generic name
  // to a resolved type if one is present. Generic names
  // all must begin with a dollar sign ($).
  std::map<std::string, CompileType*> genericMappings;
  std::vector<CompileType*> typeArgumentsList;
  std::vector<TypeMember*> fields;
  std::vector<TypeMember*> methods;

 public:
  enum class CommonType {
    BASE,
    BOOL,
    FLOAT,
    FUNCTION,
    INT,
    LIST,
    INT_RANGE,
    STRING,
    VOID,
  };

  CompileType();
  //  CompileType(const CompileType& compileType);
  CompileType(const std::string& typeName);
  CompileType(const std::string& typeName, bool isGeneric);
  CompileType(CommonType commonType);

  /*
   * makeGenericParam:
   * Factory method to make new generic type.
   */
  static CompileType* makeGenericParam(const std::string& name,
                                       CompileType* parent);
  CompileType const* getParent() const;
  void setParent(CompileType* parent);

  void addMethod(const std::string& name, CompileType* funcType);
  void addField(const std::string& name, CompileType* fieldType);
  /*
   * addGeneric:
   * Adds generic with the given name and parent.
   */
  void addGeneric(const std::string& genericName, CompileType* parent);
  /*
   * getMethodIndex: Returns index into struct's vtable where a method lies.
   * Returns -1 if method is not found.
   */
  int getMethodIndex(const std::string& name, CompileType* methodType) const;
  CompileType const* getMethodType(const std::string& name) const;
  // getFieldIndex: Returns index into struct where a field lies
  // Returns -1 if field is not found.
  int getFieldIndex(const std::string& name, CompileType* fieldType) const;

  /*
   * getGenericType:
   * Returns CompileType corresponding to genericName, or null if one doesn't
   * exist.
   */
  CompileType* getGenericType(const std::string& genericName) const;

  /*
   * getCommonTypeName:
   * Returns string value of type name for common types.
   */
  static std::string getCommonTypeName(CommonType commonType);
  std::string getTypeName() const;

  static bool isVoidType(const std::string& typeName);
  static bool isVoidType(CommonType commonType);
  static bool isVoidType(CompileType const* compileType);
  std::vector<CompileType*> const* getArgumentsList() const;
  /* getVtableLength: returns size of virtual method table in
   * number of methods.
   */
  size_t getVtableLength();
  // insertArgumentsList: adds element to back of type arguments vector.
  void insertArgumentsList(CompileType* compileType);
  void setArgumentsList(std::vector<CompileType*>* argsList);
  bool isGenericType() const;
  bool isFunctionType() const;
  /*
   * isEqualoType:
   * performs recursive type assertions to determine if
   * two types are the same.
   */
  bool isEqualToType(CompileType const* testType) const;

  /*
   * isCompatibleWithType:
   * Checks whether the incompleteType can be treated as the completeType
   *(this).
   * For example, if a complete type is List of Ints and the incomplete
   * type is List, then this function will return true.
   *
   * If both types are complete and the same,
   * then this function returns true.
   *
   * If the incomplete type itself is null, then this
   * function returns true.
   *
   * If complete type is less complete than the incomplete type,
   * this function returns false.
   */
  bool isCompatibleWithType(CompileType* incompleteType);

  /*
   * Returns true if typeA equals typeB, or if it is a subtype of typeB.
   */
  static bool isTypeOrSubtype(CompileType const* typeA,
                              CompileType const* typeB);

  /*
   * Returns true if typeA is an implemented type (not a generic type)
   * that can safely implement generic typeB. If typeA is a generic type
   * or typeB is not a generic type, this function will throw an exception.
   */
  static bool typeImplementsGeneric(CompileType const* typeA,
                                    CompileType const* typeB);
  /*
   * getFunctionReturnType:
   * Gets return type of this function type assuming this type is a function
   * type.
   */
  CompileType const* getFunctionReturnType() const;
  static CompileType const* getFunctionReturnType(
      CompileType const* functionType);
  static int VTABLE_OBJECT_INIT_INDEX;
  static int VTABLE_STRUCT_LOCATION;  // Used to get vtable in Base struct.
  static std::string BASE_TYPE_STRUCT_NAME;
  /*
   * asRawFunctionType:
   * Converts compileType into an LLVM function type. If includeThisPointer is
   * true, an extra argument for the this pointer will be created at the
   * beginning of the resulting FunctionType.
   */
  static llvm::FunctionType* asRawFunctionType(CompileType const* compileType,
                                               llvm::Type* voidStarTy,
                                               llvm::LLVMContext& currContext,
                                               bool includeThisPointer = false);

  std::string to_string() const;
  static std::string to_string(const CompileType& compileType);
  static std::string to_string(CompileType* compileType);
};

class CompileVal {
 private:
  llvm::Value* rawValue;
  CompileType compileType;

 public:
  CompileVal(llvm::Value* rawValue, std::string typeName);
  CompileVal(llvm::Value* rawValue, CompileType const* compileType);
  CompileVal(llvm::Value* rawValue, CompileType::CommonType commonType);
  CompileType* getCompileType();
  void setCompileType(CompileType* compileType);
  void insertArgumentType(CompileType* compileType);
  void setArgumentsList(std::vector<CompileType*>* argsList);
  llvm::Value* getRawValue();
  bool typesAreEqual(CompileVal* valB);
};

class TypeMember {
 private:
  std::string memberName;
  CompileType const* memberType;
  int flags;

 public:
  enum FlagBit {
    NONE = 0x0,
    STATIC = 0x1,
  };
  TypeMember(const std::string& memberName, CompileType const* compileType);
  std::string getMemberName() const;
  CompileType const* getMemberType() const;
  void setMemberType(CompileType const* compileType);
};

}  // namespace codegen
#endif
