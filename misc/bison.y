%code requires {
  #include <list>
  #include <string>
}
%{
  #include "assembler.hpp"
  #include <cstdio>
  #include <cstdlib>
  #include <cstring>
  #include <list>
  //Dodati fajlove za asembler
  extern int yylex();
  extern int errorCount;
  extern char* outFile;

  bool seenEnd = false;
  void yyerror(const char* s);
  Assembler assembler;
  /* pravi novi string "-<txt>" za negativne literale */
  static char* negLiteral(const char* txt) {
    char* res = (char*)malloc(strlen(txt) + 2);
    res[0] = '-';
    strcpy(res + 1, txt);
    return res;
  } 
%}

%union {
  int num;
  char* str;
  std::list<std::string>* list;
}

/*Registri*/
%token<num> GP_REG CSR_REG

/*Interpunkcija*/
%token LS_BRACKET RS_BRACKET COMMA PLUS MINUS DOLLAR

/*Naredbe*/
%token HALT INT IRET CALL RET JMP BEQ BNE BGT PUSH POP XCHG
%token ADD SUB MUL DIV NOT AND OR XOR SHL SHR LD ST CSRRD CSRWR

/*Direktive*/
%token GLOBAL SECTION WORD SKIP ASCII END

/* ---------- terminali sa vrednoscu ---------- */
%token<str> SYMBOL LITERAL LABEL STRING
%token ENDL

/* ---------- neterminali sa vrednoscu ---------- */
%type<str>  literal word_item
%type<list> symbol_list word_list

%%

program
  : /*prazno*/
  | program line
  ;

line 
  : ENDL
  | label ENDL
  | label content ENDL
  | content ENDL
  | error ENDL {yyerrok;}
  ;

label
  : LABEL { assembler.addLabel($1); free($1);}
  ;

content
  : instruction
  | directive
  ;

literal
  : LITERAL { $$ = $1; }
  | MINUS LITERAL { $$ = negLiteral($2); free($2); }
  ;

symbol_list
  : SYMBOL { $$ = new std::list<std::string>(); $$->push_back($1); free($1); }               
  | symbol_list COMMA SYMBOL { $1->push_back($3); free($3); $$ = $1; }
  ;

word_item
  : SYMBOL { $$ = $1; }
  | literal { $$ = $1; }
  ;

word_list
  : word_item                   { $$ = new std::list<std::string>(); $$->push_back($1); free($1);}
  | word_list COMMA word_item   { $1->push_back($3); free($3); $$ = $1; }
  ;

/*DIREKTIVE*/

directive
  : GLOBAL symbol_list {
      assembler.addGlobalSymbol(*$2);
      delete $2;
    }
  | WORD word_list{
      assembler.addWord(*$2);
      delete $2;
    }
  | SECTION SYMBOL{
      assembler.addSection($2);
      free($2);
    }
  | SKIP literal{
      assembler.addSkip($2);
      free($2);
    }
  | ASCII STRING{
      assembler.addAscii($2);
      free($2);
    }
  | END{
      seenEnd = true;
      assembler.setFileName(outFile);
      assembler.end();
      YYACCEPT;
    }
  ;
/*NAREDBE*/

instruction
  /*bez operanada*/
  : HALT {assembler.addNoMemOpInstruction(Assembler::HALT, 0);}
  | INT  {assembler.addNoMemOpInstruction(Assembler::INT, 0);}
  | IRET {assembler.addNoMemOpInstruction(Assembler::IRET, 0);}
  | RET  {assembler.addNoMemOpInstruction(Assembler::RET, 0);}

  /*poziv potprograma*/
  | CALL literal {assembler.addCallInstruction(Assembler::JUMP_OPERAND::LITERAL, $2); free($2);}
  | CALL SYMBOL {assembler.addCallInstruction(Assembler::JUMP_OPERAND::SYMBOL, $2); free($2);}
  
  /*bezuslovni skok*/
  | JMP literal {assembler.addJMPInstruction(Assembler::JUMP_OPERAND::LITERAL, $2); free($2);}
  | JMP SYMBOL {assembler.addJMPInstruction(Assembler::JUMP_OPERAND::SYMBOL, $2); free($2);}

  /*uslovni skokovi*/
  | BEQ GP_REG COMMA GP_REG COMMA literal {assembler.addBEQInstruction(Assembler::JUMP_OPERAND::LITERAL, $2, $4, $6); free($6);}
  | BEQ GP_REG COMMA GP_REG COMMA SYMBOL {assembler.addBEQInstruction(Assembler::JUMP_OPERAND::SYMBOL, $2, $4, $6); free($6);}
  | BNE GP_REG COMMA GP_REG COMMA literal {assembler.addBNEInstruction(Assembler::JUMP_OPERAND::LITERAL, $2, $4, $6); free($6);}
  | BNE GP_REG COMMA GP_REG COMMA SYMBOL {assembler.addBNEInstruction(Assembler::JUMP_OPERAND::SYMBOL, $2, $4, $6); free($6);}
  | BGT GP_REG COMMA GP_REG COMMA literal {assembler.addBGTInstruction(Assembler::JUMP_OPERAND::LITERAL, $2, $4, $6); free($6);}
  | BGT GP_REG COMMA GP_REG COMMA SYMBOL {assembler.addBGTInstruction(Assembler::JUMP_OPERAND::SYMBOL, $2, $4, $6); free($6);}

  /*stek*/
  | PUSH GP_REG {assembler.addNoMemOpInstruction(Assembler::PUSH, $2);}
  | POP GP_REG {assembler.addNoMemOpInstruction(Assembler::POP, $2);}

  /*razmena*/
  | XCHG GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::XCHG, $2, $4); }

  /*aritmetika*/
  | ADD GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::ADD, $4, $2);}
  | SUB GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::SUB, $4, $2);}
  | MUL GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::MUL, $4, $2);}
  | DIV GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::DIV, $4, $2);}


  /*logika*/
  | NOT GP_REG {assembler.addASLInstruction(Assembler::NOT, $2, $2);}
  | AND GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::AND, $4, $2);}
  | OR GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::OR, $4, $2);}
  | XOR GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::XOR, $4, $2);}

  /*pomeranje*/
  | SHL GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::SHL, $4, $2);}
  | SHR GP_REG COMMA GP_REG {assembler.addASLInstruction(Assembler::SHR, $4, $2);}

  /*LD svih 8 notacija operanda*/
  | LD DOLLAR literal COMMA GP_REG {assembler.addLDInstruction(Assembler::OPERAND::LITERAL, $5, 0, $3); free($3);}
  | LD DOLLAR SYMBOL COMMA GP_REG {assembler.addLDInstruction(Assembler::OPERAND::SYMBOL, $5, 0, $3); free($3);}
  | LD literal COMMA GP_REG {assembler.addLDInstruction(Assembler::OPERAND::MEM_LITERAL, $4, 0, $2); free($2);}
  | LD SYMBOL COMMA GP_REG {assembler.addLDInstruction(Assembler::OPERAND::MEM_SYMBOL, $4, 0, $2); free($2);}
  | LD GP_REG COMMA GP_REG {assembler.addLDInstruction(Assembler::OPERAND::REG_DIR, $4, $2);}
  | LD LS_BRACKET GP_REG RS_BRACKET COMMA GP_REG {assembler.addLDInstruction(Assembler::OPERAND::REG_IND, $6, $3);}
  | LD LS_BRACKET GP_REG PLUS literal RS_BRACKET COMMA GP_REG {assembler.addLDInstruction(Assembler::OPERAND::REG_IND_LITERAL, $8, $3, $5); free($5);}
  | LD LS_BRACKET GP_REG PLUS SYMBOL RS_BRACKET COMMA GP_REG {assembler.addLDInstruction(Assembler::OPERAND::REG_IND_SYMBOL, $8, $3, $5); free($5);}

  /*ST bez '$' jer nema smisla*/
  | ST GP_REG COMMA literal {assembler.addSTInstruction(Assembler::OPERAND::MEM_LITERAL, 0, $2, $4); free($4);}
  | ST GP_REG COMMA SYMBOL{assembler.addSTInstruction(Assembler::OPERAND::MEM_SYMBOL, 0, $2, $4); free($4);}
  | ST GP_REG COMMA GP_REG {assembler.addSTInstruction(Assembler::OPERAND::REG_DIR, $4, $2);}
  | ST GP_REG COMMA LS_BRACKET GP_REG RS_BRACKET {assembler.addSTInstruction(Assembler::OPERAND::REG_IND, $5, $2);}
  | ST GP_REG COMMA LS_BRACKET GP_REG PLUS literal RS_BRACKET {assembler.addSTInstruction(Assembler::OPERAND::REG_IND_LITERAL, $5, $2, $7); free($7);}
  | ST GP_REG COMMA LS_BRACKET GP_REG PLUS SYMBOL RS_BRACKET {assembler.addSTInstruction(Assembler::OPERAND::REG_IND_SYMBOL, $5, $2, $7); free($7);}

  /*Kontrolni i statusni registri*/
  | CSRRD CSR_REG COMMA GP_REG {assembler.addCsrInstruction(Assembler::CSRRD, $4, $2);}
  | CSRWR GP_REG COMMA CSR_REG {assembler.addCsrInstruction(Assembler::CSRWR, $2, $4);}
  ;
%%

void yyerror(const char*) {
  errorCount++;
}