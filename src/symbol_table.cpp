#include "symbol_table.hpp"

Symbol &SymbolTable::getOrCreate(const std::string &name)
{
  Symbol* symbol = find(name);
  if(symbol != nullptr){
    return *symbol;
  }
  Symbol newSymbol;
  newSymbol.name = name;
  uint32_t index = add(newSymbol);
  return symbols[index];
}

Symbol *SymbolTable::find(const std::string &name)
{
  auto it = symbolByName.find(name);
  if(it==symbolByName.end()){
    return nullptr;
  }
  return &symbols[it->second];
}

Symbol &SymbolTable::at(uint32_t index)
{
  return symbols[index];
}

uint32_t SymbolTable::add(Symbol symbol)
{
  uint32_t index = symbols.size();
  symbols.push_back(symbol);
  symbolByName[symbol.name] = index;
  return index;
}

uint32_t SymbolTable::getIndex(const std::string &name)
{
  return symbolByName.at(name);
}

const std::vector<Symbol> &SymbolTable::getSymbols() const
{
  return symbols;
}

size_t SymbolTable::size() const
{
  return symbols.size();
}
