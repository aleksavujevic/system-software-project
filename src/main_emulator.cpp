#include <cstdio>
#include <string>
#include "emulator.hpp"

int main(int argc, char **argv){
  
  if(argc==1){
    fprintf(stderr, "GRESKA: Morate navesti ulazni fajl kao argument");
    return 1;
  }
  else if(argc > 2){
    fprintf(stderr, "GRESKA: Mozete navesti samo 1 ulazni fajl");
    return 1;
  }
  const char* inFile = argv[1];
  Emulator emulator;
  emulator.emulate(inFile);
}