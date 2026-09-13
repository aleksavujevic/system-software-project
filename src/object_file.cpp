#include "object_file.hpp"

#include <fstream>
#include <cstring>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <cstdint>
using namespace std;

const char* const SPECIAL_ID = "AVSS";
const uint32_t VERSION = 1;
const uint32_t UND = UINT32_MAX;

enum flags{
  SYM_GLOBAL = 1,
  SYM_DEFINED = 2,
  SYM_SECTION = 4
};

static void write32(ostream& out, uint32_t v) {
  char b[4] = { char(v & 0xFF), char((v >> 8) & 0xFF), char((v >> 16) & 0xFF), char((v >> 24) & 0xFF) };
  out.write(b, 4);
}

static uint32_t read32(istream& in) {
  unsigned char b[4];
  in.read((char*)(b), 4);
  return ((uint32_t)b[3] << 24) | ((uint32_t)b[2] << 16) | ((uint32_t)b[1] << 8) | (uint32_t)b[0] ;
}

static void writeString(ostream& out, const string& s) {
  write32(out, (uint32_t)s.size());
  out.write(s.data(), (streamsize)s.size());
}

static string readString(istream& in) {
  uint32_t size = read32(in);
  string str(size, '\0');
  if (size) in.read(&str[0], (streamsize)size);
  return str;
}

static void error(const string& msg) {
  fprintf(stderr, "GRESKA: %s\n", msg.c_str());
  exit(1);
}
static void printSymbolTable(ostream& out, ObjectFile& object){
  const int numWidth = 5, valueWidth = 7, typeWidth = 7, bindWidth = 6, ndxWidth = 5,nameWidth = 30;
  out<<left<<setw(numWidth)<<"Num"<<left<<setw(valueWidth)<<"Value"<<left<<setw(typeWidth)<<"Type"<<left<<setw(bindWidth)<<"Bind"<<left<<setw(ndxWidth)<<"Ndx"<<left<<setw(nameWidth)<<"Name"<<endl;
  const vector<Symbol>& symbols = object.symbols.getSymbols();
  for (size_t i = 0; i<symbols.size();i++) {
    const Symbol& s = symbols[i];
    out<<left<<dec<<setw(numWidth)<<i;
    out<<left<<hex<<setw(valueWidth)<<s.value;
    out<<left<<dec<<setw(typeWidth)<<(s.isSection?"SCTN":"NOTYP");
    out<<left<<setw(bindWidth)<<(s.isGlobal?"GLOB":"LOC");
    if(s.sectionIndex==-1){
      out<<left<<setw(ndxWidth)<<"UND";
    }else{
      Section& section = object.sections.at(s.sectionIndex);
      out<<left<<setw(ndxWidth)<<section.symbolIndex;
    }
    out<<left<<setw(nameWidth)<<s.name<<endl;   
  }
}
static void printSectionTables(ostream& out, ObjectFile& object){
  for(Section& section : object.sections.getSections()) {
    out<<endl<<"section - "<<section.name<< " size: "<<hex<<section.data.size()<<'h'<<endl;
    for(size_t i = 0; i < section.data.size();i++){
      if(i%8==0){
        out<<right<<setfill('0')<<setw(4)<<i<<": ";
      }
      out<<setw(2)<<(int)section.data[i]<<((i % 8 == 7) ? "\n" : " ");
    }
    if(section.data.size()%8!=0) out<<endl;
  }
}
static void printRelocationTables(ostream& out, ObjectFile& object){
  const int offsetWidth = 8, symbolWidth = 8, addendWidth = 8;
  out<<endl<<setfill(' ')<<"Relocation Tables"<<endl;
  for (Section& section : object.sections.getSections()) {
    if (section.relocations.empty()) continue;
    out<<endl<<"section - "<<section.name<<endl;
    out<<left<<setw(offsetWidth)<<"Offset"<<setw(symbolWidth)<<"Symbol"<<setw(addendWidth)<<"Addend"<<endl;
    for(Relocation& relocation:section.relocations){
      out<<left<<setw(offsetWidth)<<relocation.offset<<setw(symbolWidth)<<relocation.symbolIndex<<setw(addendWidth)<<relocation.addend<<endl;
    }
  }
}

ObjectFile readObjectFile(const string &path)
{
  ifstream in(path, ios::binary);
  if(!in.is_open()) error("Ne mogu da otvorim ulaznu datoteku '"+path+"'");

  char id[4];
  in.read(id,4);
  if(!in||memcmp(id,SPECIAL_ID,4) != 0){
    error("Datoteka '"+path+"' nije predmetni program");
  }
  uint32_t fileVersion = read32(in);
  if(fileVersion != VERSION){
    error("Datoteka '"+path+"' nije adekvatne verzije");
  }

  uint32_t numSections = read32(in);
  uint32_t numSymbols = read32(in);

  ObjectFile object;

  vector<uint32_t> symbolNdx;

  //Tabela simbola
  for(uint32_t i = 0; i < numSymbols; i++){
    Symbol symbol;
    symbol.value = read32(in);
    symbolNdx.push_back(read32(in));
    uint32_t flags = read32(in);
    symbol.isGlobal = flags&SYM_GLOBAL;
    symbol.isDefined = flags&SYM_DEFINED;
    symbol.isSection = flags&SYM_SECTION;
    symbol.name = readString(in);
    object.symbols.add(symbol);
  }

  //Sekcije
  for(uint32_t i = 0; i < numSections; i++){
    uint32_t symbolIndex = read32(in);
    string name = readString(in);
    Section& section = object.sections.add(name, symbolIndex);
    uint32_t size = read32(in);
    section.data.resize(size);
    if(size) in.read((char*)(section.data.data()), (streamsize)size);
    //Relokacije za tekucu sekciju
    uint32_t numRelocations = read32(in);
    for(uint32_t j = 0; j < numRelocations; j++){
      Relocation relocation;
      relocation.offset = read32(in);
      relocation.symbolIndex = read32(in);
      relocation.addend = (int32_t)read32(in);
      section.relocations.push_back(relocation);
    }
    Symbol& sectionSymbol = object.symbols.at(symbolIndex);
    if(!sectionSymbol.isSection)
      error("Simbol sekcije '"+name+"' nije tipa SCTN");
    sectionSymbol.sectionIndex = i;
  }

  for(uint32_t i =0; i<object.symbols.size(); i++){
    Symbol& symbol = object.symbols.at(i);
    uint32_t ndx = symbolNdx[i];
    if(ndx == UND) symbol.sectionIndex = -1;
    else{
      Symbol& sectionSymbol = object.symbols.at(ndx);
      symbol.sectionIndex = sectionSymbol.sectionIndex;
    }
  }

  if (!in) error("Datoteka '" + path + "' je oštećena ili nepotpuna");
  in.peek();
  if (!in.eof()) error("Datoteka '" + path + "' ima visak podataka na kraju");
  return object;
}

void writeObjectFileBin(const string &path, ObjectFile &object)
{
  ofstream out(path, ios::binary);
  if (!out.is_open()) error("Ne mogu da otvorim izlaznu datoteku '" + path + "'");
  out.write(SPECIAL_ID, 4);
  write32(out, VERSION);
  write32(out, object.sections.size());
  write32(out, object.symbols.size());
  //Tabela simbola
  const vector<Symbol>& symbols = object.symbols.getSymbols();
  for (uint32_t i =0; i<symbols.size(); i++) {
    const Symbol& symbol = symbols[i];
    write32(out, symbol.value);
    if(symbol.sectionIndex == -1) write32(out, UND);
    else {
      if(symbol.sectionIndex<0 || (uint32_t)symbol.sectionIndex>=object.sections.size()){
        error("Simbol '"+symbol.name+"' pokazuje na nepostojecu sekciju");
      }
      Section& section = object.sections.at(symbol.sectionIndex);
      write32(out,section.symbolIndex);
    }
    uint32_t flags = 0;
    if(symbol.isGlobal) flags = flags | SYM_GLOBAL;
    if(symbol.isDefined) flags = flags | SYM_DEFINED;
    if(symbol.isSection) flags = flags | SYM_SECTION;
    write32(out, flags);
    writeString(out, symbol.name);
  }
  //Sekcije
  for(Section& section : object.sections.getSections()){
    write32(out, section.symbolIndex);
    writeString(out, section.name);
    write32(out, section.data.size());
    for (uint8_t byte : section.data) out.put((char)byte);
    write32(out, section.relocations.size());
    for(Relocation& relocation : section.relocations){
      write32(out, relocation.offset);
      write32(out, relocation.symbolIndex);
      write32(out, (uint32_t)relocation.addend);
    }
  }
  if(!out){
    error("Greska pri upisu datoteke '"+path+"'");
  }
}

void writeObjectFileTxt(const string &path, ObjectFile &object)
{
  ofstream out(path);
  if(!out.is_open()){
    error("Ne mogu da otvorim datoteku '"+path+"'");
  }
  printSymbolTable(out, object);
  printSectionTables(out, object);
  printRelocationTables(out,object);
}
