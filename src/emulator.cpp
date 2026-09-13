#include "emulator.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <unistd.h>

void Emulator::error(const string& msg) {
  fprintf(stderr, "GRESKA: %s\n", msg.c_str());
  exit(1);
}

uint8_t Emulator::read8(uint32_t adr){
  auto it = memory.find(adr);
  return (it == memory.end())?0:it->second;
}

uint32_t Emulator::read32(uint32_t adr)
{
  if(adr == TERM_IN) return termIn;
  return ((uint32_t)read8(adr+3)<<24) | ((uint32_t)read8(adr+2)<<16) | ((uint32_t)read8(adr+1)<<8) | (uint32_t)read8(adr);
}

void Emulator::write32(uint32_t adr, uint32_t val)
{
  if(adr == TERM_OUT){
    putchar((char)(val&0xFF));
    fflush(stdout);
    return;
  }
  memory[adr+0] = (uint8_t)(val & 0xFF);
  memory[adr+1] = (uint8_t)((val>>8) & 0xFF);
  memory[adr+2] = (uint8_t)((val>>16) & 0xFF);
  memory[adr+3] = (uint8_t)((val>>24) & 0xFF);
}

void Emulator::pushOnStack(uint32_t val)
{
  gpr[SP]-=4;
  write32(gpr[SP],val);
}

void Emulator::executeOne()
{
  uint32_t rec = read32(gpr[PC]);
  gpr[0] = 0;
  gpr[PC] = gpr[PC] + 4;
  int oc = (rec>>28) & 0xF;
  int mod = (rec>>24) & 0xF;
  int A = (rec>>20) & 0xF;
  int B = (rec>>16) & 0xF;
  int C = (rec>>12) & 0xF;
  int32_t D = rec & 0xFFF;
  if(D & 0x800)D=D|0xFFFFF000;

  switch (oc){
    case HALT:
      stopped = true;
      break;
    case INT:
      interrupt(4);
      break;
    case CALL:
      execCall(mod,A,B,D);
      break;
    case JMP:
      execJmp(mod,A,B,C,D);
      break;
    case XCHG:
      execXchg(B,C);
      break;
    case ARITHMETIC:
      execArithmetic(mod,A,B,C); 
      break;
    case LOGICAL:
      execLogical(mod,A,B,C);
      break;
    case SHIFT:
      execShift(mod,A,B,C);
      break;
    case STORE:
      execStore(mod,A,B,C,D);
      break;
    case LOAD:
      execLoad(mod,A,B,C,D);
      break;
    default:
      interrupt(1);
  }
}

void Emulator::interrupt(uint32_t cause)
{
  pushOnStack(csr[STATUS]);
  pushOnStack(gpr[PC]);
  csr[CAUSE] = cause;
  csr[STATUS] = csr[STATUS] | 0x4u; //Maskirani spoljasni prekidi
  gpr[PC] = csr[HANDLER];
}

void Emulator::setTerminal()
{
  tcgetattr(STDIN_FILENO, &oldTermios);
  struct termios t = oldTermios;
  t.c_lflag &= ~(ICANON | ECHO); //Bez entera i bez echa
  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void Emulator::resetTerminal()
{
  tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
}

void Emulator::checkKeyboard()
{
  char c;
  if(read(STDIN_FILENO, &c, 1)==1){
    termIn = (uint8_t)c;
    keyPending = true;
  }
  if(keyPending && !(csr[STATUS] & 0x4) && !(csr[STATUS] & 0x2)){
    keyPending = false;
    interrupt(3);
  }
}

void Emulator::execCall(int mod, int regA, int regB, int32_t disp)
{
  if(mod==CALL_DISP){
    pushOnStack(gpr[PC]);
    gpr[PC] = gpr[regA]+gpr[regB]+disp;
  }
  else if(mod == CALL_POOL){
    pushOnStack(gpr[PC]);
    gpr[PC] = read32(gpr[regA]+gpr[regB]+disp);
  }
  else{
    interrupt(1);
  }
}

void Emulator::execJmp(int mod, int regA, int regB, int regC, int32_t disp)
{
  switch (mod){
    case JMP_DISP:
      gpr[PC] = gpr[regA]+disp; 
      break;
    case BEQ_DISP:
      if(gpr[regB] == gpr[regC]) gpr[PC] = gpr[regA]+disp; 
      break;
    case BNE_DISP:
      if(gpr[regB] != gpr[regC]) gpr[PC] = gpr[regA]+disp; 
      break;
    case BGT_DISP:
      if((int32_t)gpr[regB] > (int32_t)gpr[regC]) gpr[PC] = gpr[regA]+disp; 
      break;
    case JMP_POOL:
      gpr[PC] = read32(gpr[regA]+disp); 
      break;
    case BEQ_POOL:
      if(gpr[regB] == gpr[regC]) gpr[PC] = read32(gpr[regA]+disp); 
      break;
    case BNE_POOL:
      if(gpr[regB] != gpr[regC]) gpr[PC] = read32(gpr[regA]+disp); 
      break;
    case BGT_POOL:
      if((int32_t)gpr[regB] > (int32_t)gpr[regC]) gpr[PC] = read32(gpr[regA]+disp); 
      break;
    default:
      interrupt(1);
  }
}

void Emulator::execXchg(int regB, int regC)
{
  uint32_t temp = gpr[regB];
  gpr[regB] = gpr[regC];
  gpr[regC] = temp;
}

void Emulator::execArithmetic(int mod, int regA, int regB, int regC)
{
  switch (mod){
    case ADD:
      gpr[regA] = gpr[regB] + gpr[regC];
      break;
    case SUB:
      gpr[regA] = gpr[regB] - gpr[regC];
      break;
    case MUL:
      gpr[regA] = gpr[regB] * gpr[regC];
      break;
    case DIV:
      gpr[regA] = gpr[regB] / gpr[regC];
      break;
    default:
      interrupt(1);
  }
}

void Emulator::execLogical(int mod, int regA, int regB, int regC)
{
  switch (mod){
    case NOT:
      gpr[regA] = ~gpr[regB];
      break;
    case AND:
      gpr[regA] = gpr[regB] & gpr[regC];
      break;
    case OR:
      gpr[regA] = gpr[regB] | gpr[regC];
      break;
    case XOR:
      gpr[regA] = gpr[regB] ^ gpr[regC];
      break;
    default:
      interrupt(1);
  }
}

void Emulator::execShift(int mod, int regA, int regB, int regC)
{
  if(mod == ASL){
    gpr[regA] = gpr[regB] << gpr[regC];
  }
  else if(mod == ASR){
    gpr[regA] = gpr[regB] >> gpr[regC];
  }
  else{
    interrupt(1);
  }
}

void Emulator::execStore(int mod, int regA, int regB, int regC, int32_t disp)
{
  if(mod == ST_MEM_DIR){
    write32(gpr[regA]+gpr[regB]+disp, gpr[regC]);
  }
  else if(mod == ST_MEM_IND){
    write32(read32(gpr[regA]+gpr[regB]+disp), gpr[regC]);
  }
  else if(mod == PUSH){
    gpr[regA] = gpr[regA]+disp;
    write32(gpr[regA], gpr[regC]);
  }
  else{
    interrupt(1);
  }
}

void Emulator::execLoad(int mod, int regA, int regB, int regC, int32_t disp)
{
  switch (mod){
    case CSRRD:
      gpr[regA] = csr[regB];
      break;
    case LD_IMMED:
      gpr[regA]=gpr[regB]+disp;
      break;
    case LD_MEM_DIR:
      gpr[regA]=read32(gpr[regB]+gpr[regC]+disp);
      break;
    case POP:
      gpr[regA]=read32(gpr[regB]);
      gpr[regB]=gpr[regB]+disp;
      break;
    case CSRWR:
      csr[regA] = gpr[regB];
      break;
    case CSR_OR:
      csr[regA] = csr[regB] | disp;
      break;
    case LD_CSR_MEM_DIR:
      csr[regA] = read32(gpr[regB]+gpr[regC]+disp);
      break;
    case POP_CSR:
      csr[regA] = read32(gpr[regB]);
      gpr[regB] = gpr[regB] + disp;
      break; 
    default:
      interrupt(1);
  }
}

void Emulator::loadHex(const string &path)
{
  ifstream in(path);
  if(!in) error("Emulator ne moze da otvori fajl '"+path+"'");

  string linija;
  while(getline(in,linija)){
    size_t dvotacka = linija.find(':');
    uint32_t adr = (uint32_t)strtoul(linija.substr(0, dvotacka).c_str(), nullptr, 16);

    istringstream ostatak(linija.substr(dvotacka+1));
    string bajt;
    while(ostatak>>bajt){
      memory[adr]=(uint8_t)strtoul(bajt.c_str(), nullptr, 16);
      adr++;
    }
  }
}

void Emulator::printRegisters()
{
  cout<<"\n-----------------------------------------------------------------\n";
  cout<<"Emulated processor executed halt instruction\n";
  cout<<"Emulated processor state:\n";
  for(int i = 0; i<16; i++){
    cout<<setfill(' ')<<right<<setw(3)<<"r"+to_string(i)<<'='<<"0x"<<setw(8)<<uppercase<<hex<<setfill('0')<<gpr[i];
    if((i+1)%4==0){
      cout<<'\n';
    }
    else{
      cout<<"   ";
    }
  }
}

void Emulator::emulate(const string &path)
{
  loadHex(path);
  gpr[PC] = START_ADR;
  setTerminal();
  while(!stopped){
    checkKeyboard();
    executeOne();
  }
  resetTerminal();
  printRegisters();
}