#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include <string>
#include <map>
#include <termios.h>
#include <cstdint>
using namespace std;

class Emulator{
public:
  void emulate(const string &path);
  static const int SP = 14, PC = 15, STATUS = 0, HANDLER = 1, CAUSE = 2;

  static const uint32_t START_ADR = 0x40000000;
  static const uint32_t TERM_OUT = 0xFFFFFF00;
  static const uint32_t TERM_IN = 0xFFFFFF04;

private:
  map<uint32_t, uint8_t> memory;
  uint32_t gpr[16] = {0};
  uint32_t csr[3] = {0};
  bool stopped = false;

  void loadHex(const string& path);
  void printRegisters();
  void error(const string& msg);

  uint8_t read8(uint32_t adr);
  uint32_t read32(uint32_t adr);
  void write32(uint32_t adr, uint32_t v);
  void pushOnStack(uint32_t val);
  void executeOne();
  void interrupt(uint32_t cause);


  struct termios oldTermios;
  bool keyPending = false;
  uint32_t termIn = 0;
  void setTerminal();
  void resetTerminal();
  void checkKeyboard();

  void execCall(int mod, int regA, int regB, int32_t disp);
  void execJmp(int mod, int regA, int regB, int regC, int32_t disp);
  void execXchg(int regB, int regC);
  void execArithmetic(int mod, int regA, int regB, int regC);
  void execLogical(int mod, int regA, int regB, int regC);
  void execShift(int mod, int regA, int regB, int regC);
  void execStore(int mod, int regA, int regB, int regC, int32_t disp);
  void execLoad(int mod, int regA, int regB, int regC, int32_t disp);
  enum INS {
    HALT = 0b0000,
    INT = 0b0001,
    CALL = 0b0010,
    JMP = 0b0011,
    XCHG = 0b0100,
    ARITHMETIC = 0b0101,
    LOGICAL = 0b0110,
    SHIFT = 0b0111,
    STORE = 0b1000,
    LOAD = 0b1001
  };

  enum CALL_INS{
    CALL_DISP = 0b0000,
    CALL_POOL = 0b0001
  };

  enum JMP_INS{
    JMP_DISP = 0b0000,
    BEQ_DISP = 0b0001,
    BNE_DISP = 0b0010,
    BGT_DISP = 0b0011,
    JMP_POOL = 0b1000,
    BEQ_POOL = 0b1001,
    BNE_POOL = 0b1010,
    BGT_POOL = 0b1011,
  };

  enum ARITHMETIC_INS{
    ADD = 0b0000,
    SUB = 0b0001,
    MUL = 0b0010,
    DIV = 0b0011
  };

  enum LOGICAL_INS{
    NOT = 0b0000,
    AND = 0b0001,
    OR = 0b0010,
    XOR = 0b0011
  };

  enum SHIFT_INS{
    ASL = 0b0000,
    ASR = 0b0001
  };

  enum STORE_INS{
    ST_MEM_DIR = 0b0000,
    ST_MEM_IND = 0b0010,
    PUSH = 0b0001
  };

  enum LOAD_INS{
    CSRRD = 0b0000,
    LD_IMMED = 0b0001,
    LD_MEM_DIR = 0b0010,
    POP = 0b0011,
    CSRWR = 0b0100,
    CSR_OR = 0b0101,
    LD_CSR_MEM_DIR = 0b0110,
    POP_CSR = 0b0111
  };
};
#endif