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

class CompileType {
 private:
  std::string typeName;
  CompileType* parent;
  // genericMappings contains mappings from a generic name
  // to a resolved type if one is present. Generic names
  // all must begin with a dollar sign ($).
  std::map<std::string, CompileType*> genericMappings;
  std::vector<CompileType*> typeArgumentsList;
  std::vector<std::tuple<std::string, CompileType*>> fields;
  std::vector<std::tuple<std::string, CompileType*>> methods;

 public:
  enum class CommonType {
    BASE,
    BOOL,
    FLOAT,
    FUNCTION,
    INT,
    LIST,
    STRING,
    VOID,
  };

  CompileType();
  CompileType(const std::string& typeName);
  CompileType(CommonType commonType);

  CompileType* getParent();
  void setParent(CompileType* parent);

  void addMethod(const std::string& name, CompileType* funcType);
  void addField(const std::string& name, CompileType* fieldType);
  // getMethodIndex: Returns index into struct's vtable where a method lies.
  // Returns -1 if method is not found.
  int getMethodIndex(const std::string& name, CompileType* methodType);
  // getFieldIndex: Returns index into struct where a field lies
  // Returns -1 if field is not found.
  int getFieldIndex(const std::string& name, CompileType* fieldType);

  /*
   * getCommonTypeName:
   * Returns string value of type name for common types.
   */
  static std::string getCommonTypeName(CommonType commonType);
  std::string getTypeName();

  static bool isVoidType(const std::string& typeName);
  static bool isVoidType(CommonType commonType);
  static bool isVoidType(CompileType* compileType);
  std::vector<CompileType*>* getArgumentsList();
  // getVtableLength: returns size of virtual method table in
  // number of methods.
  size_t getVtableLength();
  // insertArgumentsList: adds element to back of type arguments vector.
  void insertArgumentsList(CompileType* compileType);
  void setArgumentsList(std::vector<CompileType*>* argsList);
  bool isFunctionType();
  /*
   * isEqualoType:
   * performs recursive type assertions to determine if
   * two types are the same.
   */
  bool isEqualToType(CompileType* testType);

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
   * getFunctionReturnType:
   * Gets return type of this function type assuming this type is a function
   * type.
   */
  CompileType* getFunctionReturnType();
  static CompileType* getFunctionReturnType(CompileType* functionType);
  static int VTABLE_OBJECT_INIT_INDEX;
  static int VTABLE_STRUCT_LOCATION;  // Used to get vtable in Base struct.
  static std::string BASE_TYPE_STRUCT_NAME;
  /*
   * asRawFunctionType:
   * Converts compileType into an LLVM function type. If includeThisPointer is
   * true, an extra argument for the this pointer will be created at the
   * beginning of the resulting FunctionType.
   */
  static llvm::FunctionType* asRawFunctionType(CompileType* compileType,
                                               llvm::Type* voidStarTy,
                                               bool includeThisPointer = false);
};

class CompileVal {
 private:
  llvm::Value* rawValue;
  CompileType compileType;

 public:
  CompileVal(llvm::Value* rawValue, std::string typeName);
  CompileVal(llvm::Value* rawValue, CompileType* compileType);
  CompileVal(llvm::Value* rawValue, CompileType::CommonType commonType);
  CompileType* getCompileType();
  void setCompileType(CompileType* compileType);
  void insertArgumentType(CompileType* compileType);
  void setArgumentsList(std::vector<CompileType*>* argsList);
  llvm::Value* getRawValue();
  bool typesAreEqual(CompileVal* valB);
};
}  // namespace codegen
#endif
