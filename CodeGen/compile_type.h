#ifndef COMPILE_TYPE_H
#define COMPILE_TYPE_H

/*
 * compile_type manages type information. The CompileType class deals with
 * storing
 * and enforcing objects at compile time. The CompileVal class deals with
 * specific
 * instances of a type.
 */

#include <vector>
#include <string>

#include "llvm/IR/Value.h"

class CompileType {
 private:
  std::string typeName;
  std::vector<CompileType*> genericsList;

 public:
  enum class CommonType {
    BOOL,
    FLOAT,
    FUNCTION,
    INT,
    LIST,
    STRING,
    VOID,
  };

  CompileType(std::string typeName);
  CompileType(CommonType commonType);

  /*
   * getCommonTypeName:
   * Returns string value of type name for common types.
   */
  static std::string getCommonTypeName(CommonType commonType);
  std::string getTypeName();

  static bool isVoidType(std::string typeName);
  static bool isVoidType(CommonType commonType);
  static bool isVoidType(CompileType* compileType);
  // isArgument: returns true if this type has generic types within it
  bool isArgument();
  std::vector<CompileType*>* getArgumentsList();
  // insertArgumentsList: adds element to back of generics vector
  void insertArgumentsList(CompileType* compileType);
  void setArgumentsList(std::vector<CompileType*>* argsList);
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

#endif
