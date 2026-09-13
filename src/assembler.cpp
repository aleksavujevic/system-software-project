#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <string>
#include "assembler.hpp"
#include "object_file.hpp"

void Assembler::error(const string& msg) {
  fprintf(stderr, "GRESKA: %s\n", msg.c_str());
  exit(1);
}

void Assembler::setFileName(const std::string &name)
{
  filename = name;
  filenameTXT = name.substr(0, name.size()-2)+".txt";
}

int32_t Assembler::parseLiteral(const string &str)
{
  bool negative = false;
  size_t i = 0;
  if(i<str.size() && str[i]=='-'){
    negative = true; 
    i++;
  }
  else if(i<str.size() && str[i]=='+'){
    i++;
  }
  int base = 10;
  if(i+1<str.size() && str[i] == '0' && (str[i+1]=='x'||str[i+1]=='X')){
    base = 16;
    i = i + 2;
  }
  long v = strtol(str.c_str()+i,nullptr,base);
  return (int32_t)(negative ? -v : v);
}

void Assembler::addSection(const string &name)
{
  finishSection();
  Symbol& symbol = symbolTable.getOrCreate(name);

  if(symbol.isDefined)
    error("Ime sekcije '"+name+"' vec definisano");
  uint32_t sectionIndex = sectionTable.size();
  sectionTable.add(name, symbolTable.getIndex(name));
  sectionData.push_back(AssemblerSectionData());
  curSection = sectionIndex;
  symbol.sectionIndex = sectionIndex;
  symbol.value = 0;
  symbol.isDefined = true;
  symbol.isSection = true;
}

void Assembler::addLabel(const string &label)
{
  if(curSection < 0) error("Labela ne sme biti van sekcije");
  Symbol& symbol = symbolTable.getOrCreate(label);
  if(symbol.isDefined) error("Visestruka definicija simbola '"+label+"'");
  symbol.sectionIndex = curSection;
  symbol.value = cur().locationCounter();
  symbol.isDefined = true;
}

void Assembler::addGlobalSymbol(const list<string> &symbols)
{
  for(string s : symbols){
    symbolTable.getOrCreate(s).isGlobal = true;
  }
}

void Assembler::addSkip(const string &literal)
{
  if(curSection < 0) error("Skip ne sme biti van sekcije");
  int n = parseLiteral(literal);
  if(n<0) error("Skip ne sme da koristi negativne literale");
  for(int i = 0; i<n; i++){
    writeByte(0);
  }
}

void Assembler::addWord(const list<string> &items)
{
  if(curSection < 0) error("Word ne sme biti van sekcije");
  for(string s : items){
    if(isdigit((unsigned char)s[0]) || s[0]=='-' || s[0]=='+'){
      write32(parseLiteral(s));
    }
    else{
      addRelocation(cur(), cur().locationCounter(), s);
      write32(0);
    }
  }
}

void Assembler::addAscii(const string &text)
{
  if(curSection < 0) error("Ascii ne sme biti van sekcije");
  for(size_t i = 0; i<text.length();i++){
    if(text[i]=='\\' && i+1<text.length()){
      i = i + 1;
      switch (text[i])
      {
      case 'n': writeByte('\n'); break;
      case 't': writeByte('\t'); break;
      case 'r': writeByte('\r'); break;
      default: writeByte(text[i]); break;
      }
    }
    else{
      writeByte(text[i]);
    }
  }
}

void Assembler::end()
{
  if(errorCount > 0){
    error("Postoji "+to_string(errorCount)+"sintaksnih gresaka");
  }
  finishSection();
  resolveRelocations();
  ObjectFile object;
  object.symbols = symbolTable;
  object.sections = sectionTable;
  writeObjectFileBin(filename, object);
  writeObjectFileTxt(filenameTXT, object);
}

void Assembler::addASLInstruction(OPS operation, uint8_t regD, uint8_t regS)
{
  switch (operation){
    //Aritmeticke
    case ADD:
      writeInstruction(0b0101, 0b0000, regD, regD, regS, 0);
      break;
    case SUB:
      writeInstruction(0b0101, 0b0001, regD, regD, regS, 0);
      break;
    case MUL:
      writeInstruction(0b0101, 0b0010, regD, regD, regS, 0);
      break;
    case DIV:
      writeInstruction(0b0101, 0b0011, regD, regD, regS, 0);
      break;
    //Logicke
    case NOT:
      writeInstruction(0b0110, 0b0000, regD, regS, 0, 0);
      break;
    case AND:
      writeInstruction(0b0110, 0b0001, regD, regD, regS, 0);
      break;
    case OR:
      writeInstruction(0b0110, 0b0010, regD, regD, regS, 0);
      break;
    case XOR:
      writeInstruction(0b0110, 0b0011, regD, regD, regS, 0);
      break;
    //Pomeracke
    case SHL:
      writeInstruction(0b0111, 0b0000, regD, regD, regS, 0);
      break;
    case SHR:
      writeInstruction(0b0111, 0b0001, regD, regD, regS, 0);
      break;
    //Change
    case XCHG:
      writeInstruction(0b0100, 0b0000, 0, regD, regS, 0);
      break;
    default:
      error("Instrukcija '"+to_string(operation)+"' ne spada u grupu ASL instrukcija");
      break;
  }
}

void Assembler::addNoMemOpInstruction(OPS operation, uint8_t reg)
{
  switch(operation){
    case HALT:
      writeInstruction(0b0000, 0b0000, 0, 0, 0, 0);
      break;
    case INT:
      writeInstruction(0b0001, 0b0000, 0, 0, 0, 0);
      break;
    case IRET:
      writeInstruction(0b1001, 0b0110, 0, 14, 0, 4);
      writeInstruction(0b1001, 0b0011, 15, 14, 0, 8);
      break;
    case RET:
      writeInstruction(0b1001, 0b0011, 15, 14, 0, 4);
      break;
    case PUSH:
      writeInstruction(0b1000, 0b0001, 14, 0, reg, -4);
      break;
    case POP:
      writeInstruction(0b1001, 0b0011, reg, 14, 0, 4);
      break;
    default:
      error("Instrukcija '"+to_string(operation)+"' ne spada u datu grupu instrukcija");
      break;
  }
}

void Assembler::addCsrInstruction(OPS operation, uint8_t gpr, uint8_t csr)
{
  switch(operation){
    case CSRRD:
      writeInstruction(0b1001, 0b0000, gpr, csr, 0, 0);
      break;
    case CSRWR:
      writeInstruction(0b1001, 0b0100, csr, gpr, 0, 0);
      break;
    default:
      error("Instrukcija '"+to_string(operation)+"' ne spada u CSR grupu instrukcija");
      break;
  }
}

void Assembler::addCallInstruction(JUMP_OPERAND operand, const string &val)
{
  addJumpInstruction(operand, 0b0010, 0b0000, 0b0001, 0, 0, val);
}

void Assembler::addJMPInstruction(JUMP_OPERAND operand, const string &val)
{
  addJumpInstruction(operand, 0b0011, 0b0000, 0b1000, 0, 0, val);
}

void Assembler::addBEQInstruction(JUMP_OPERAND operand, uint8_t gpr1, uint8_t gpr2, const string &val)
{
  addJumpInstruction(operand, 0b0011, 0b0001, 0b1001, gpr1, gpr2, val);
}

void Assembler::addBNEInstruction(JUMP_OPERAND operand, uint8_t gpr1, uint8_t gpr2, const string &val)
{
  addJumpInstruction(operand, 0b0011, 0b0010, 0b1010, gpr1, gpr2, val);
}

void Assembler::addBGTInstruction(JUMP_OPERAND operand, uint8_t gpr1, uint8_t gpr2, const string &val)
{
  addJumpInstruction(operand, 0b0011, 0b0011, 0b1011, gpr1, gpr2, val);
}

void Assembler::addJumpInstruction(JUMP_OPERAND operand, uint8_t opCode, uint8_t dirMode, uint8_t poolMode, uint8_t gpr1, uint8_t gpr2, const string &val)
{
  switch (operand){
    case JUMP_OPERAND::LITERAL:{
      long value = parseLiteral(val);
      if(canFitIn12Bits(value)){
        writeInstruction(opCode, dirMode, 0, gpr1, gpr2, value);
      }
      else{
        addFixup(addPoolLiteral(value));
        writeInstruction(opCode, poolMode, 15, gpr1, gpr2, 0);
      }
      break;
    }
    case JUMP_OPERAND::SYMBOL:
      addFixup(addPoolSymbol(val));
      writeInstruction(opCode, poolMode, 15, gpr1, gpr2, 0);
      break;
    default:
      error("Instrukcija skoka ima nedozvoljen tip operanda");
      break;
  }
}

void Assembler::addLDInstruction(OPERAND operand, uint8_t gprD, uint8_t gprS, const string &val)
{
  switch (operand)
  {
    case OPERAND::LITERAL:{
      long value = parseLiteral(val);
      if(canFitIn12Bits(value)){
        writeInstruction(0b1001, 0b0001, gprD, 0, 0, value);
      }
      else{
        addFixup(addPoolLiteral(value));
        writeInstruction(0b1001, 0b0010, gprD, 15, 0, 0);
      }
      break;
    }
    case OPERAND::SYMBOL:
      addFixup(addPoolSymbol(val));
      writeInstruction(0b1001, 0b0010, gprD, 15, 0, 0);
      break;
    case OPERAND::MEM_LITERAL:{
      long value = parseLiteral(val);
      if(canFitIn12Bits(value)){
        writeInstruction(0b1001, 0b0010, gprD, 0, 0, value);
      }
      else{
        addFixup(addPoolLiteral(value));
        writeInstruction(0b1001, 0b0010, gprD, 15, 0, 0);
        writeInstruction(0b1001, 0b0010, gprD, gprD, 0, 0);
      }
      break;
    }
    case OPERAND::MEM_SYMBOL:
      addFixup(addPoolSymbol(val));
      writeInstruction(0b1001, 0b0010, gprD, 15, 0, 0);
      writeInstruction(0b1001, 0b0010, gprD, gprD, 0, 0);
      break;
    case OPERAND::REG_DIR:
      writeInstruction(0b1001, 0b0001, gprD, gprS, 0, 0);
      break;
    case OPERAND::REG_IND:
      writeInstruction(0b1001, 0b0010, gprD, gprS, 0, 0);
      break;
    case OPERAND::REG_IND_LITERAL:{
      long value = parseLiteral(val);
      if(canFitIn12Bits(value)){
        writeInstruction(0b1001, 0b0010, gprD, gprS, 0, value);
      }
      else{
        error("Pomeraj kod [%<reg>+<literal>] mora stati u 12 bita");
      }
      break;
    }
    case OPERAND::REG_IND_SYMBOL:
      error("Adresiranje [%<reg>+<symbol>] nije moguce bez .equ direktive");
      break;
    default:
      error("Instrukcija 'LD' ima nedozvoljen tip operanda");
      break;
  }
}

void Assembler::addSTInstruction(OPERAND operand, uint8_t gprD, uint8_t gprS, const string &val)
{
  switch (operand)
  {
    case OPERAND::MEM_LITERAL:{
      long value = parseLiteral(val);
      if(canFitIn12Bits(value)){
        writeInstruction(0b1000, 0b0000, 0, 0, gprS, value);
      }
      else{
        addFixup(addPoolLiteral(value));
        writeInstruction(0b1000, 0b0010, 15, 0, gprS, 0);
      }
      break;
    }
    case OPERAND::MEM_SYMBOL:
      addFixup(addPoolSymbol(val));
      writeInstruction(0b1000, 0b0010, 15, 0, gprS, 0);
      break;
    case OPERAND::REG_DIR:
      writeInstruction(0b1001, 0b0001, gprD, gprS, 0, 0);
      break;
    case OPERAND::REG_IND:
      writeInstruction(0b1000, 0b0000, gprD, 0, gprS, 0);
      break;
    case OPERAND::REG_IND_LITERAL:{
      long value = parseLiteral(val);
      if(canFitIn12Bits(value)){
        writeInstruction(0b1000, 0b0000, gprD, 0, gprS, value);
      }
      else{
        error("Pomeraj kod [%<reg>+<literal>] mora stati u 12 bita");
      }
      break;
    }
    case OPERAND::REG_IND_SYMBOL:
      error("Adresiranje [%<reg>+<symbol>] nije moguce bez .equ direktive");
      break;
    default:
      error("Instrukcija 'ST' ima nedozvoljen tip operanda");
      break;
  }
}

int Assembler::addPoolLiteral(int32_t value)
{
  AssemblerSectionData& data = sectionData[curSection];
  for(size_t i = 0; i<data.literalPool.size();i++){
    if(data.literalPool[i].isLiteral && data.literalPool[i].value == value){
      return (int)i;
    }
  }
  LiteralPool newEntry;
  newEntry.isLiteral = true;
  newEntry.value = value;
  data.literalPool.push_back(newEntry);
  return (int)data.literalPool.size()-1;
}

int Assembler::addPoolSymbol(const string &name)
{
  AssemblerSectionData& data = sectionData[curSection];
  for(size_t i = 0; i<data.literalPool.size();i++){
    if(!data.literalPool[i].isLiteral && data.literalPool[i].symName == name){
      return (int)i;
    }
  }
  LiteralPool newEntry;
  newEntry.isLiteral = false;
  newEntry.symName = name;
  data.literalPool.push_back(newEntry);
  return (int)data.literalPool.size()-1;
}

void Assembler::addFixup(int poolIndex)
{
  AssemblerSectionData& data = sectionData[curSection];
  Fixup newFix;
  newFix.instrOffset = cur().locationCounter();
  newFix.poolIndex = poolIndex;
  data.fixups.push_back(newFix);
}

void Assembler::finishSection()
{
  if (curSection < 0) return; //Prva sekcija - ne radi nista
  Section& section = cur();
  AssemblerSectionData& data = sectionData[curSection];
  for(size_t i = 0; i < data.literalPool.size();i++){
    LiteralPool& poolEntry = data.literalPool[i];
    poolEntry.offset = (int)section.data.size();
    if(poolEntry.isLiteral){
      write32(poolEntry.value);
    }
    else{
      addRelocation(section, poolEntry.offset, poolEntry.symName);
      write32(0);
    }
  }

  for(const Fixup& fixup: data.fixups){
    int disp = data.literalPool[fixup.poolIndex].offset - (fixup.instrOffset + 4);
    if(!canFitIn12Bits(disp)) error("Bazen je predaleko od instrukcije");
    section.data[fixup.instrOffset] = disp & 0xFF;
    section.data[fixup.instrOffset+1] = ((section.data[fixup.instrOffset+1])& 0xF0) | ((disp>>8)&0xF);
  }
}

void Assembler::addRelocation(Section &section, uint32_t offset, const string &symName)
{
  symbolTable.getOrCreate(symName);
  Relocation newEntry;
  newEntry.offset = offset;
  newEntry.symbolIndex = symbolTable.getIndex(symName);
  section.relocations.push_back(newEntry);
}

void Assembler::resolveRelocations()
{
  for(Section& section : sectionTable.getSections()){
    for(Relocation& relocation: section.relocations){
      Symbol& symbol = symbolTable.at(relocation.symbolIndex);

      if(!symbol.isDefined && !symbol.isGlobal)
        error("Nerazresen simbol '"+symbol.name+"'");
      
      if(!symbol.isGlobal){
        Section& symbolSection = sectionTable.at(symbol.sectionIndex);
        relocation.symbolIndex = symbolSection.symbolIndex;
        relocation.addend = symbol.value;
      }
    }
  }
}

Section &Assembler::cur(){
  return sectionTable.at(curSection);
}

void Assembler::writeByte(uint8_t b) { cur().data.push_back(b); }

void Assembler::writeInstruction(int opCode, int mode, int regA, int regB, int regC, int disp)
{
  if(curSection < 0) error("Instrukcija ne sme biti van sekcije");
  writeByte(disp & 0xFF);
  writeByte(((regC & 0xF)<<4) | ((disp>>8) & 0xF));
  writeByte(((regA & 0xF)<<4) | (regB & 0xF));
  writeByte(((opCode & 0xF)<<4) | (mode & 0xF));
}

void Assembler::write32(int32_t v) {
  writeByte(v & 0xFF);
  writeByte((v >> 8) & 0xFF);
  writeByte((v >> 16) & 0xFF);
  writeByte((v >> 24) & 0xFF);
}