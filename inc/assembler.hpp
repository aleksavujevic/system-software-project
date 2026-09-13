#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <string>
#include <vector>
#include <list>
#include <cstdint>
#include "symbol_table.hpp"
#include "section_table.hpp"
extern int errorCount;
using namespace std;

class Assembler {
public:
  enum OPS {HALT, INT, IRET, CALL, RET, JMP, BEQ, BNE, BGT, PUSH, POP, XCHG, 
            ADD, SUB, MUL, DIV, NOT, AND, OR, XOR, SHL, SHR, LD, ST, CSRRD, CSRWR};
  
  enum class JUMP_OPERAND {LITERAL, SYMBOL};

  enum class OPERAND {LITERAL, SYMBOL, MEM_LITERAL, MEM_SYMBOL, REG_DIR, REG_IND, REG_IND_LITERAL, REG_IND_SYMBOL};

  struct LiteralPool{
    bool isLiteral = false; //Za literale = true, za simbole = false
    int32_t value = 0;
    string symName;
    int offset = -1;
  };

  struct Fixup{
    int instrOffset;
    int poolIndex;
  };

  struct AssemblerSectionData{
    vector<LiteralPool> literalPool;
    vector<Fixup> fixups;
  };

  void setFileName(const std::string& name);

  static int32_t parseLiteral(const string& s);

  void addSection(const string& name);
  void addLabel(const string& label);
  void addGlobalSymbol(const list<string>& symbols);
  void addSkip(const string& literal);
  void addWord(const list<string>& items);
  void addAscii(const string& text);
  void end();

  void addASLInstruction(OPS op, uint8_t gprD, uint8_t gprS);
  void addNoMemOpInstruction(OPS op, uint8_t reg);
  void addCsrInstruction(OPS op, uint8_t gpr, uint8_t csr);

  void addCallInstruction(JUMP_OPERAND operand, const string& val);
  void addJMPInstruction(JUMP_OPERAND operand, const string& val);
  void addBEQInstruction(JUMP_OPERAND operand, uint8_t gpr1, uint8_t gpr2, const string& val);
  void addBNEInstruction(JUMP_OPERAND operand, uint8_t gpr1, uint8_t gpr2, const string& val);
  void addBGTInstruction(JUMP_OPERAND operand, uint8_t gpr1, uint8_t gpr2, const string& val);
  
  void addLDInstruction(OPERAND operand, uint8_t gprD, uint8_t gprS, const string& val = "");
  void addSTInstruction(OPERAND operand, uint8_t gprD, uint8_t gprS, const string& val = "");
private:
  string filename = "izlaz.o";
  string filenameTXT = "izlaz.txt";
  SymbolTable symbolTable;
  SectionTable sectionTable;
  vector<AssemblerSectionData> sectionData;
  int curSection = -1;

  Section& cur();

  void writeByte(uint8_t b);
  void writeInstruction(int opCode, int mode, int regA, int regB, int regC, int disp);
  void write32(int32_t v);

  void error(const string& msg);

  bool canFitIn12Bits(long v) { return (v>=-2048 && v<=2047);}

  int addPoolLiteral(int32_t v);
  int addPoolSymbol(const string& name);
  void addFixup(int poolIndex);

  void finishSection();

  void addRelocation(Section& section, uint32_t offset, const string& symName);
  void resolveRelocations();

  void addJumpInstruction(JUMP_OPERAND operand, uint8_t opCode, uint8_t dirMode, uint8_t poolMode, uint8_t gpr1, uint8_t gpr2, const string& val);
};
#endif