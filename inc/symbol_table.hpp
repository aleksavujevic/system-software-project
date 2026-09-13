#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <map>
struct Symbol{
  std::string name;

  uint32_t value = 0;
  int32_t sectionIndex = -1;

  bool isGlobal = false;
  bool isDefined = false;
  bool isSection = false;
};

class SymbolTable{
public:
  Symbol& getOrCreate(const std::string& name);
  Symbol* find(const std::string& name);
  Symbol& at(uint32_t index);
  uint32_t add(Symbol symbol);
  uint32_t getIndex(const std::string& name);
  const std::vector<Symbol>& getSymbols() const;
  size_t size() const;


private:
  std::vector<Symbol> symbols;
  std::map<std::string, uint32_t> symbolByName;
};

#endif
