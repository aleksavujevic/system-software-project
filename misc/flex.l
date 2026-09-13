%{
  #include "bison.tab.hpp"
  #include <cstring>
  #include <cstdlib>
  #include <cstdio>
  int errorCount = 0;
%}

%option noyywrap
%option nounput noinput

%%

 /*Komentari*/
"#"[^\n\r]* {;}

 /*Registri*/
"%r"([0-9]|1[0-5]) {yylval.num = atoi(yytext + 2); return GP_REG;}
"%sp"              {yylval.num = 14; return GP_REG;}
"%pc"              {yylval.num = 15; return GP_REG;}
"%status"          {yylval.num = 0; return CSR_REG;}
"%handler"         {yylval.num = 1; return CSR_REG;}
"%cause"           {yylval.num = 2; return CSR_REG;}

 /*Direktive*/
".global"|".extern"  {return GLOBAL;}
".section" {return SECTION;}
".word"    {return WORD;}
".skip"    {return SKIP;}
".ascii"   {return ASCII;}
".end"     {return END;}

 /*Naredbe*/
"halt"  {return HALT;}
"int"   {return INT;}
"iret"  {return IRET;}
"call"  {return CALL;}
"ret"   {return RET;}
"jmp"   {return JMP;}
"beq"   {return BEQ;}
"bne"   {return BNE;}
"bgt"   {return BGT;}
"push"  {return PUSH;}
"pop"   {return POP;}
"xchg"  {return XCHG;}
"add"   {return ADD;}
"sub"   {return SUB;}
"mul"   {return MUL;}
"div"   {return DIV;}
"not"   {return NOT;}
"and"   {return AND;}
"or"    {return OR;}
"xor"   {return XOR;}
"shl"   {return SHL;}
"shr"   {return SHR;}
"ld"    {return LD;}
"st"    {return ST;}
"csrrd" {return CSRRD;}
"csrwr" {return CSRWR;}

 /*Literali*/
(0[xX][0-9a-fA-F]+|[0-9]+) {yylval.str = strdup(yytext); return LITERAL;}

 /*Labela*/
[a-zA-Z_.][a-zA-Z0-9_.]*":" {
  size_t len = yyleng - 1;
  yylval.str = (char*)malloc(len+1);
  memcpy(yylval.str, yytext, len);
  yylval.str[len] = '\0';
  return LABEL;
}

 /*String za .ascii*/
"\""([^"\n\r]|\\.)*"\""  {
  size_t len = yyleng - 2;
  yylval.str = (char*)malloc(len+1);
  memcpy(yylval.str, yytext+1, len);
  yylval.str[len] = '\0';
  return STRING;
}

 /*Interpunkcija*/
"[" {return LS_BRACKET;}
"]" {return RS_BRACKET;}
"," {return COMMA;}
"+" {return PLUS;}
"-" {return MINUS;}
"$" {return DOLLAR;}

 /*Simbol*/
[a-zA-Z_.][a-zA-Z0-9_.]*  {yylval.str = strdup(yytext); return SYMBOL;}

 /*Endline*/
\r?\n {return ENDL;}

 /*Blank space*/
[ \t\r]+  {;}

 /*Kraj fajla*/
<<EOF>> {
  static int eofSeen = 0;
  if(!eofSeen){eofSeen = 1; return ENDL;}
  return 0;
}

 /*Sve ostalo je greska*/
. {errorCount++;}
%%