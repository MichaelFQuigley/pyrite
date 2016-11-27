/*
 * type_reader.h
 *
 * Reads in type information from a file.
 * One type or value is specified per line.
 *
 * Author: Michael Quigley
 *
 * Grammar for encoding info looks as follows:
 *
 * line: (class | field | method) '\n'
 * class: 'C' typename ':' typename
 * method: 'M' typename? '.' IDENTIFIER '$' args typename
 * field: 'F' typename? '.' IDENTIFIER '$' typename
 * typename: IDENTIFIER ('<' IDENTIFIER '>')?
 * args: (IDENTIFIER '$')*
 * IDENTIFIER: any valid function or field name.
 *
 */

#ifndef TYPE_READER_H
#define TYPE_READER_H

#include <fstream>
#include <vector>
#include <map>
#include "compile_type.h"

namespace codegen {

class TypeReader {
 private:
  std::map<std::string, CompileType*> compileTypes;
  // TODO allow for overloading of methods in globals map.
  std::map<std::string, CompileType*> globals;
  void initializeClasses(const std::vector<std::string>& lines);
  void initializeSuperclasses(const std::vector<std::string>& lines);
  void initializeMethods(const std::vector<std::string>& lines);
  void initializeFields(const std::vector<std::string>& lines);
  static std::string getClassName(const std::string& classString, char endChar);
  static std::string getSuperclassName(const std::string& classString);
  void assertHasClass(const std::string& canonicalClassName);
  CompileType* parseMethodType(const std::string& methodString);
  static void assertStringIndexExists(std::string errorMsg,
                                      std::size_t stringIndex);
  static std::string getMemberName(const std::string& memberString);
  // accessTypeFromFullTypeName:
  // Checks map to see if a type with the provided name is available.
  // The typeName passed in includes the generic arguments if available.
  // The containingClassName is the containing class name of a method or field
  // that is composed of the type indicated by typeName. containingClassName may
  // be null
  // if there is no such class.
  CompileType* accessTypeFromFullTypeName(std::string containingClassName,
                                          const std::string& typeName);

 public:
  std::vector<std::string> classLines;
  std::vector<std::string> methodLines;
  std::vector<std::string> fieldLines;
  void compileTypesFromFile(std::ifstream& inFile);
  static const char CLASS_START;
  static const char FIELD_START;
  static const char METHOD_START;
  static const char GENERIC_START;
  static const char GENERIC_END;
  static const char SUPERCLASS_START;
  static const char CONTAINING_CLASS_END;
  static const char MEMBER_TYPE_START;
};

}  // namespace codegen

#endif
