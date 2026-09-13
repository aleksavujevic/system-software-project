#include <cstdio>
#include <cstring>

extern int   yyparse();
extern bool seenEnd;
extern FILE* yyin;

char* outFile = 0;    /* koristi ga bison.y */

int main(int argc, char** argv) {
  const char* inFile = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 >= argc) { fprintf(stderr, "GRESKA: -o zahteva naziv\n"); return 1; }
      outFile = argv[++i];
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "GRESKA: nepoznata opcija '%s'\n", argv[i]); return 1;
    } else {
      inFile = argv[i];
    }
  }

  if (!inFile) {
    fprintf(stderr, "Upotreba: %s [-o <izlaz>] <ulaz.s>\n", argv[0]);
    return 1;
  }
  if (!outFile) outFile = (char*)"izlaz.o";

  yyin = fopen(inFile, "r");
  if (!yyin) { fprintf(stderr, "GRESKA: ne mogu da otvorim '%s'\n", inFile); return 1; }

  int status = yyparse();
  if(!seenEnd){fprintf(stderr, "GRESKA: .end simbol nije vidjen"); return 1;}
  fclose(yyin);
  return status;
}