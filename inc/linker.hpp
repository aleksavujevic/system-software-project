#ifndef LINKER_HPP
#define LINKER_HPP
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "object_file.hpp"

using namespace std;

class Linker{
public:
  void addObjFile(const string& path);
  void addPlace(const string& ime, uint32_t adresa);
  void setOutputFile(const string& path) { outputFile = path; }
  void setHex(bool v) { hexMode = v; }
  void link();
private:
struct InputFile{
  ObjectFile object;
  vector<uint32_t> sectionOffset;
  vector<uint32_t> symbolRemap;
};
struct SectionPlacement{
  uint32_t baseAddress = 0;
  bool placed = false;
};
  string outputFile = "izlaz.hex";
  bool hexMode = false;
  vector<string> inputPaths;
  vector<InputFile> inputFiles;
  map<string, uint32_t> placeOptions;
  ObjectFile mergedObject;

  map<string, uint32_t> globalSymbolIndex;
  vector<SectionPlacement> sectionPlacements;
  
  void error(const string& msg);
  void loadInputFiles();
  void mergeSections();
  void mergeSymbols();
  void mergeRelocations();
  void checkUndefinedSymbols();
  void placeSections();
  void applyRelocations();
  void outputHex();
  void outputRelocatable();
  void write32(Section& section, uint32_t offset, uint32_t value);

  void defineOutputSymbol(Symbol& outputSymbol, const Symbol& inputSymbol, InputFile& input);
};
#endif