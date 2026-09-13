#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <utility>
#include "linker.hpp"

void Linker::error(const string& msg) {
  fprintf(stderr, "GRESKA: %s\n", msg.c_str());
  exit(1);
}

void Linker::loadInputFiles()
{
  for(const string& path : inputPaths){
    InputFile input;
    input.object = readObjectFile(path);
    input.sectionOffset.resize(input.object.sections.size());
    input.symbolRemap.resize(input.object.symbols.size());
    inputFiles.push_back(input);
  }
}

void Linker::mergeSections()
{
  for(InputFile& input : inputFiles){
    for(uint32_t i = 0; i<input.object.sections.size(); i++){
      Section& inputSection = input.object.sections.at(i);
      Section* outputSection = mergedObject.sections.find(inputSection.name);
      if(outputSection==nullptr){
        Symbol sectionSymbol;
        sectionSymbol.name = inputSection.name;
        sectionSymbol.isDefined = true;
        sectionSymbol.isSection = true;
        sectionSymbol.sectionIndex = (int32_t)mergedObject.sections.size();
        uint32_t newSymbolIndex = mergedObject.symbols.add(sectionSymbol);
        outputSection = &mergedObject.sections.add(inputSection.name, newSymbolIndex);
      }
      input.sectionOffset[i]=(uint32_t)outputSection->data.size();
      outputSection->data.insert(outputSection->data.end(),inputSection.data.begin(), inputSection.data.end());
      input.symbolRemap[inputSection.symbolIndex] = outputSection->symbolIndex;
    }
  }
}

void Linker::mergeSymbols()
{
  for(InputFile& input : inputFiles){
    for(uint32_t i = 0; i < input.object.symbols.size();i++){
      Symbol& inputSymbol = input.object.symbols.at(i);
      if(inputSymbol.isSection) continue;
      if(!inputSymbol.isGlobal){
        Symbol outputSymbol = inputSymbol;
        defineOutputSymbol(outputSymbol, inputSymbol, input);
        uint32_t outputIndex = mergedObject.symbols.add(outputSymbol);
        input.symbolRemap[i] = outputIndex;
        continue;
      }
      auto it = globalSymbolIndex.find(inputSymbol.name);
      if(it==globalSymbolIndex.end()){
        Symbol outputSymbol = inputSymbol;
        if(inputSymbol.isDefined){
          defineOutputSymbol(outputSymbol, inputSymbol, input);
        }
        uint32_t outputIndex = mergedObject.symbols.add(outputSymbol);
        globalSymbolIndex[inputSymbol.name]=outputIndex;
        input.symbolRemap[i]=outputIndex;
        continue;
      }

      uint32_t outputIndex = it->second;
      Symbol& outputSymbol = mergedObject.symbols.at(outputIndex);
      if(!inputSymbol.isDefined){
        input.symbolRemap[i] = outputIndex;
        continue;
      }
      if(outputSymbol.isDefined){
        error("Visestruka definicija simbola '"+inputSymbol.name+"'");
      }
      defineOutputSymbol(outputSymbol, inputSymbol, input);
      input.symbolRemap[i] = outputIndex;
    }
  }
}

void Linker::mergeRelocations()
{
  for(InputFile& input : inputFiles){
    for(uint32_t sectionIndex = 0; sectionIndex<input.object.sections.size();sectionIndex++){
      Section& inputSection = input.object.sections.at(sectionIndex);
      Section* outputSection = mergedObject.sections.find(inputSection.name);
      for(Relocation& inputRelocation : inputSection.relocations){
        Relocation outputRelocation = inputRelocation;
        outputRelocation.offset = inputRelocation.offset + input.sectionOffset[sectionIndex];
        outputRelocation.symbolIndex = input.symbolRemap[inputRelocation.symbolIndex];
        Symbol& inputSymbol = input.object.symbols.at(inputRelocation.symbolIndex);
        if(inputSymbol.isSection){ //Ne za globalne simbole
          outputRelocation.addend = outputRelocation.addend + (int32_t)input.sectionOffset[inputSymbol.sectionIndex];
        }
        outputSection->relocations.push_back(outputRelocation);
      }
    }
  }
}

void Linker::checkUndefinedSymbols()
{
  for(const Symbol& symbol : mergedObject.symbols.getSymbols()){
    if(!symbol.isDefined){
      error("Nerazresen simbol '"+symbol.name+"'");
    }
  }
}

void Linker::addPlace(const string &ime, uint32_t adresa)
{
  if(placeOptions.count(ime)){
    error("Opcija -place je vise puta zadata za sekciju '"+ime+"'");
  }
  placeOptions[ime] = adresa;
}

void Linker::addObjFile(const string &path)
{
  inputPaths.push_back(path);
}

void Linker::placeSections()
{
  uint32_t lastAdr = 0;
  sectionPlacements.resize(mergedObject.sections.size());

  for(auto& [name, address] : placeOptions){
    Section* section = mergedObject.sections.find(name);
    if(section==nullptr){
      error("Opcija -place navodi sekciju '"+name+"' koja ne postoji u fajlu");
    }
    uint32_t sectionIndex = mergedObject.sections.getIndex(name);
    sectionPlacements[sectionIndex].baseAddress = address;
    sectionPlacements[sectionIndex].placed = true;
    uint32_t end = address+(uint32_t)section->data.size();
    if(end > lastAdr){
      lastAdr = end;
    }
  }
  for(uint32_t i = 0; i < mergedObject.sections.size();i++){
    if(!sectionPlacements[i].placed)continue;
    Section& sectionA = mergedObject.sections.at(i);
    if(sectionA.data.empty())continue;
    for(uint32_t j = i + 1; j < mergedObject.sections.size();j++){
      if(!sectionPlacements[j].placed)continue;
      Section& sectionB = mergedObject.sections.at(j);
      if(sectionB.data.empty())continue;
      uint32_t startA = sectionPlacements[i].baseAddress;
      uint32_t startB = sectionPlacements[j].baseAddress;
      uint32_t endA = startA + (uint32_t)sectionA.data.size();
      uint32_t endB = startB + (uint32_t)sectionB.data.size();
      if(startA < endB && startB < endA){
        error("Sekcije '"+sectionA.name+"' i '"+sectionB.name+"' se preklapaju");
      }
    }
  }
  for(uint32_t i = 0; i<mergedObject.sections.size(); i++){
    if(sectionPlacements[i].placed)continue;
    Section& section = mergedObject.sections.at(i);
    sectionPlacements[i].baseAddress = lastAdr;
    lastAdr+=(uint32_t)section.data.size();
  }
}

void Linker::applyRelocations()
{
  for(Section& section : mergedObject.sections.getSections()){
    for(Relocation& relocation : section.relocations){
      Symbol& symbol = mergedObject.symbols.at(relocation.symbolIndex);
      uint32_t value = sectionPlacements[symbol.sectionIndex].baseAddress + symbol.value + (uint32_t)relocation.addend; //value za globalne, addend za lokalne
      write32(section, relocation.offset, value);
    }
  }
}

void Linker::outputHex()
{
  map<uint32_t, uint8_t> mem;
  for(uint32_t sectionIndex = 0; sectionIndex < mergedObject.sections.size();sectionIndex++){
    Section& section = mergedObject.sections.at(sectionIndex);
    uint32_t baseAdr = sectionPlacements[sectionIndex].baseAddress;
    for(uint32_t i = 0; i < (uint32_t)section.data.size(); i++){
      mem[baseAdr + i] = section.data[i];
    }
  }
  ofstream out(outputFile);
  if(!out.is_open()){
    error("Ne mogu da otvorim izlaznu datoteku '"+outputFile+"'");
  }
  out<<hex<<uppercase<<setfill('0');
  bool first = true;
  uint32_t prethodna = 0;
  for(auto& [adr, bajt] : mem){
    if(first || adr!=prethodna+1 || (adr%8) == 0){
      if(!first){
        out << "\n";
      }
      out<<setw(8)<<adr<<":";
      first = false;
    }
    out<<" "<<setw(2)<<(int)bajt;
    prethodna = adr;
  }
  if(!first){
    out<<"\n";
  }
}

void Linker::link()
{
  loadInputFiles();
  mergeSections();
  mergeSymbols();
  mergeRelocations();

  if(hexMode){
    checkUndefinedSymbols();
    placeSections();
    applyRelocations();
    outputHex();
  }
  else{
    outputRelocatable();
  }
}

void Linker::outputRelocatable()
{
  writeObjectFileBin(outputFile, mergedObject);
  string txtFile = outputFile;
  size_t dot = txtFile.find_last_of('.');
  if(dot != string::npos){
    txtFile = txtFile.substr(0,dot);
  }
  txtFile += ".txt";
  writeObjectFileTxt(txtFile, mergedObject);
}

void Linker::write32(Section &section, uint32_t offset, uint32_t value)
{
  section.data[offset+0]=(uint8_t)(value&0xFF);
  section.data[offset+1]=(uint8_t)((value>>8)&0xFF);
  section.data[offset+2]=(uint8_t)((value>>16)&0xFF);
  section.data[offset+3]=(uint8_t)((value>>24)&0xFF);
}

void Linker::defineOutputSymbol(Symbol &outputSymbol, const Symbol &inputSymbol, InputFile &input)
{
  Section& inputSection = input.object.sections.at(inputSymbol.sectionIndex);
  outputSymbol.isDefined = true;
  outputSymbol.sectionIndex = mergedObject.sections.getIndex(inputSection.name);
  outputSymbol.value = inputSymbol.value + input.sectionOffset[inputSymbol.sectionIndex];
}
