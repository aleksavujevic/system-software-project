#ifndef OBJECT_FILE_HPP
#define OBJECT_FILE_HPP

#include <string>

#include "symbol_table.hpp"
#include "section_table.hpp"

struct ObjectFile{
  SymbolTable symbols;
  SectionTable sections;
};

ObjectFile readObjectFile(const std::string& path);
void writeObjectFileBin(const std::string& path, ObjectFile& object);
void writeObjectFileTxt(const std::string& path, ObjectFile& object);
#endif