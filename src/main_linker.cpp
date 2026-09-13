#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include "../inc/linker.hpp"

int main(int argc, char** argv){

  Linker linker;
  bool hex = false, rel = false;
  bool imaUlaza = false;
  vector<pair<string,uint32_t>> places;
  for(int i = 1; i < argc; i++){
    string a = argv[i];
    if(a == "-hex") {hex = true;}
    else if (a == "-relocatable") {rel = true;}
    else if (a == "-o"){
      if(i + 1 >= argc){
        fprintf(stderr, "GRESKA: Opcija -o zahteva naziv datoteke\n");
        return 1;
      }
      linker.setOutputFile(argv[++i]);
    }
    else if(a.rfind("-place=",0)==0){
      size_t at = a.find('@');
      if (at==string::npos){
        fprintf(stderr, "GRESKA: Nepravilan format opcije -place\n");
        return 1;
      }
      string ime = a.substr(7,at-7);
      uint32_t adr = (uint32_t)strtoul(a.c_str()+at+1,nullptr,0);
      places.push_back({ime,adr});
    }
    else if(a[0] == '-'){
      fprintf(stderr, "GRESKA: Nepoznata opcija '%s'\n", argv[i]);
      return 1;
    }
    else{
      linker.addObjFile(a);
      imaUlaza = true;
    }
  }

  if(hex==rel){
    fprintf(stderr, "GRESKA: Morate navesti tacno jednu opciju -hex ili -relocatable\n");
    return 1;
  }
  if(!imaUlaza){
    fprintf(stderr, "GRESKA: Morate navesti barem jednu ulaznu datoteku\n");
    return 1;
  }
  if(hex){
    for(auto& [ime, adr]:places){
      linker.addPlace(ime,adr);
    }
  }
  linker.setHex(hex);
  linker.link();
  return 0;
}