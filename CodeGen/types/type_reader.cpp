/*
 * type_reader.cpp
 *
 * Reads in type information from a file.
 * Author: Michael Quigley
 */
#include "type_reader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <iterator>

namespace codegen {

const char TypeReader::CLASS_START = 'C';
const char TypeReader::SUPERCLASS_START = ':';
const char TypeReader::FIELD_START = 'F';
const char TypeReader::METHOD_START = 'M';
const char TypeReader::GENERIC_START = '<';
const char TypeReader::GENERIC_END = '>';
const char TypeReader::CONTAINING_CLASS_END = '.';
const char TypeReader::MEMBER_TYPE_START = '$';
const char TypeReader::COMMENT_START = '#';

void TypeReader::compileTypesFromFile(std::ifstream& inFile) {
  // TODO Error handling for file param.
  std::string line;

  while (std::getline(inFile, line)) {
    std::string sanitizedStr = std::regex_replace(line, std::regex("\\s+"), "");
    if (sanitizedStr.empty()) continue;
    char metadataType = sanitizedStr[0];

    if (metadataType == TypeReader::COMMENT_START) continue;

    sanitizedStr = sanitizedStr.substr(1);
    switch (metadataType) {
      case TypeReader::CLASS_START:
        classLines.push_back(sanitizedStr);
        break;
      case TypeReader::FIELD_START:
        fieldLines.push_back(sanitizedStr);
        break;
      case TypeReader::METHOD_START:
        methodLines.push_back(sanitizedStr);
        break;
      default:
        // fail here
        break;
    }
  }

  initializeClasses(classLines);
  initializeSuperclasses(classLines);

  initializeMethods(methodLines);

  initializeFields(fieldLines);
}

void TypeReader::initializeClasses(const std::vector<std::string>& lines) {
  for (auto line : lines) {
    std::string canonicalName =
        getClassName(line, TypeReader::SUPERCLASS_START);
    CompileType* compileType = new CompileType(canonicalName);
    if (compileTypes.find(canonicalName) == compileTypes.end()) {
      compileTypes[canonicalName] = compileType;
    } else {
      throw std::runtime_error("Multiple definitions of class: " +
                               canonicalName);
    }
  }
}

void TypeReader::initializeSuperclasses(const std::vector<std::string>& lines) {
  for (auto line : lines) {
    CompileType* derivedClass =
        compileTypes[getClassName(line, TypeReader::SUPERCLASS_START)];
    std::string superclassName = getSuperclassName(line);
    if (superclassName.empty()) {
      derivedClass->setParent(nullptr);
      continue;
    }
    assertHasClass(superclassName);
    // TODO allow for superclasses with generic types.
    derivedClass->setParent(compileTypes[superclassName]);
  }
}

void TypeReader::initializeMethods(const std::vector<std::string>& lines) {
  for (auto methodString : lines) {
    std::string canonicalClassName =
        getClassName(methodString, TypeReader::CONTAINING_CLASS_END);
    std::string methodName = getMemberName(methodString);
    CompileType* methodType = parseMethodType(methodString);
    if (!canonicalClassName.empty()) {  // Class method.
      assertHasClass(canonicalClassName);
      compileTypes[canonicalClassName]->addMethod(methodName, methodType);
    } else {  // Regular function.
      if (globals.find(methodName) != globals.end()) {
        throw std::runtime_error("Multiple definitions of method: " +
                                 methodName);
      }
      globals[methodName] = methodType;
    }
  }
}

void TypeReader::initializeFields(const std::vector<std::string>& lines) {
  for (auto fieldString : lines) {
    std::string canonicalClassName =
        getClassName(fieldString, TypeReader::CONTAINING_CLASS_END);
    std::string fieldName = getMemberName(fieldString);
    std::size_t fieldTypeIndex =
        fieldString.find(TypeReader::MEMBER_TYPE_START);
    assertStringIndexExists("Invalid field string: " + fieldString,
                            fieldTypeIndex);
    std::string fieldTypeStr = fieldString.substr(fieldTypeIndex + 1);
    CompileType* fieldType =
        accessTypeFromFullTypeName(canonicalClassName, fieldTypeStr);
    if (!canonicalClassName.empty()) {  // Class field.
      assertHasClass(canonicalClassName);
      compileTypes[canonicalClassName]->addField(fieldName, fieldType);
    } else {  // Regular field.
      if (globals.find(fieldName) != globals.end()) {
        throw std::runtime_error("Multiple definitions of field: " + fieldName);
      }
      globals[fieldName] = fieldType;
    }
  }
}

CompileType* TypeReader::accessTypeFromFullTypeName(
    std::string containingClassName, const std::string& typeName) {
  std::size_t canonicalNameIndex = typeName.find(TypeReader::GENERIC_START);
  std::string canonicalTypeName = typeName;
  if (canonicalNameIndex != std::string::npos) {
    canonicalTypeName = typeName.substr(0, canonicalNameIndex);
  }
  assertHasClass(canonicalTypeName);

  if (canonicalTypeName == typeName) {
    return compileTypes[canonicalTypeName];
  } else {
    throw std::runtime_error("No support for generic arguments.");
  }
  // TODO handle containingClassName.
}

CompileType* TypeReader::parseMethodType(const std::string& methodString) {
  CompileType* resultType = new CompileType(CompileType::CommonType::FUNCTION);
  std::size_t methodArgsStartIndex =
      methodString.find(TypeReader::MEMBER_TYPE_START);
  std::string containingClassName =
      getClassName(methodString, TypeReader::CONTAINING_CLASS_END);
  assertStringIndexExists("Invalid method string: " + methodString,
                          methodArgsStartIndex);

  std::string argsStr = methodString.substr(methodArgsStartIndex + 1);
  std::size_t prevArgsStrIndex = 0;
  std::size_t currArgStrIndex = argsStr.find(TypeReader::MEMBER_TYPE_START, 0);
  // Add arguments.
  while (currArgStrIndex != std::string::npos) {
    std::string argStr =
        argsStr.substr(prevArgsStrIndex, currArgStrIndex - prevArgsStrIndex);
    CompileType* argType =
        accessTypeFromFullTypeName(containingClassName, argStr);
    resultType->insertArgumentsList(argType);
    prevArgsStrIndex = currArgStrIndex + 1;
    currArgStrIndex =
        argsStr.find(TypeReader::MEMBER_TYPE_START, prevArgsStrIndex);
  }
  // Add return type.
  std::string returnTypeStr = argsStr.substr(prevArgsStrIndex);
  CompileType* returnType =
      accessTypeFromFullTypeName(containingClassName, returnTypeStr);
  resultType->insertArgumentsList(returnType);

  return resultType;
}

void TypeReader::assertHasClass(const std::string& canonicalClassName) {
  if (compileTypes.find(canonicalClassName) == compileTypes.end()) {
    throw std::runtime_error("Use of undefined class: " + canonicalClassName);
  }
}

std::string TypeReader::getMemberName(const std::string& memberString) {
  std::size_t containingClassIndex =
      memberString.find(TypeReader::CONTAINING_CLASS_END);
  assertStringIndexExists("Invalid member string: " + memberString,
                          containingClassIndex);

  std::size_t memberTypeStartIndex =
      memberString.find(TypeReader::MEMBER_TYPE_START);
  assertStringIndexExists("Invalid member string: " + memberString,
                          memberTypeStartIndex);

  return memberString.substr(containingClassIndex + 1,
                             memberTypeStartIndex - (containingClassIndex + 1));
}

void TypeReader::assertStringIndexExists(std::string errorMsg,
                                         std::size_t stringIndex) {
  if (stringIndex == std::string::npos) {
    throw std::runtime_error(errorMsg);
  }
}

std::string TypeReader::getSuperclassName(const std::string& classString) {
  std::size_t derivedClassIndex =
      classString.find(TypeReader::SUPERCLASS_START);
  assertStringIndexExists("Invalid class string: " + classString,
                          derivedClassIndex);
  return classString.substr(derivedClassIndex + 1);
}

std::string TypeReader::getClassName(const std::string& classString,
                                     char endChar) {
  std::size_t derivedClassIndex = classString.find(endChar);
  assertStringIndexExists("Invalid class string: " + classString,
                          derivedClassIndex);
  std::string derivedClassName = classString.substr(0, derivedClassIndex);
  std::size_t canonicalNameIndex =
      derivedClassName.find(TypeReader::GENERIC_START);
  if (canonicalNameIndex != std::string::npos) {
    return derivedClassName.substr(0, canonicalNameIndex);
  }
  return derivedClassName;
}

std::map<std::string, CompileType*> TypeReader::getCompileTypesMap() const {
  return compileTypes;
}
}  // namespace codegen
