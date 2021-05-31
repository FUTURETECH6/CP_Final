%{
#include "parser.hpp"
#include "symbol.h"
#include <cctype>
#include <cstdio>
#include <cstring>
extern "C" int yylex();
extern "C" void yyerror(const char *s);
int Conversion(char *Ident, unsigned type);
%}

number          [0-9]
char            [a-zA-Z]
char_const      \'.\'
string_const    \'.*\'
Delim           [\t\n\r ]
real            ({number}+\.{number}+)|(\.{number}+)
integer         {number}+
identify        {char}(_|{char}|{number})*
ws              {Delim}+
comment         (\{.*\}|\(\*.*\*\)|\/\/.*)

%%

"("         {return Token_LeftP;}
")"         {return Token_RightP;}
"["         {return Token_LeftB;}
"]"         {return Token_RightB;}
"."         {return Token_DOT;}
","         {return Token_COMMA;}
":"         {return Token_COLON;}
"+"         {return Token_PLUS;}
"-"         {return Token_MINUS;}
"*"         {return Token_MUL;}
"/"         {return Token_DIV;}
">"         {return Token_GT;}
"<"         {return Token_LT;}
"="         {return Token_EQUAL;}
"<>"        {return Token_NE;}
"<="        {return Token_LE;}
">="        {return Token_GE;}
":="        {return Token_ASSIGN;}
"NOT"       {return Token_NOT;}
"MOD"       {return Token_MOD;}
"%"         {return Token_MOD;}
".."        {return Token_DOTDOT;}
";"         {return Token_SEMI;}

"and"       {return Token_AND;}
"array"     {return Token_ARRAY;}
"begin"     {return Token_BEGIN;}
"case"      {return Token_CASE;}
"const"     {return Token_CONST;}
"label"     {return Token_LABEL;}
"do"        {return Token_DO;}
"downto"    {return Token_DOWNTO;}
"else"      {return Token_ELSE;}
"end"       {return Token_END;}
"for"       {return Token_FOR;}
"function"  {return Token_FUNCTION;}
"goto"      {return Token_GOTO;}
"if"        {return Token_IF;}
"in"        {return Token_IN;}
"of"        {return Token_OF;}
"or"        {return Token_OR;}
"packed"    {return Token_PACKED;}
"procedure" {return Token_PROCEDURE;}
"program"   {return Token_PROGRAM;}
"record"    {return Token_RECORD;}
"repeat"    {return Token_REPEAT;}
"set"       {return Token_SET;}
"then"      {return Token_THEN;}
"to"        {return Token_TO;}
"type"      {return Token_TYPE;}
"until"     {return Token_UNTIL;}
"var"       {return Token_VAR;}
"while"     {return Token_WHILE;}
"with"      {return Token_WITH;}

"false"     {return Token_FALSE;}
"true"      {return Token_TRUE;}
"maxint"    {return Token_MAXINT;}
"abs"       {return Token_ABS;}
"chr"       {return Token_CHR;}
"odd"       {return Token_ODD;}
"ord"       {return Token_ORD;}
"pred"      {return Token_PRED;}
"sqr"       {return Token_SQR;}
"sqrt"      {return Token_SQRT;}
"succ"      {return Token_SUCC;}
"write"     {return Token_WRITE;}
"writeln"   {return Token_WRITELN;}
"read"      {return Token_READ;}
"boolean"   {return Token_BOOLEAN;}
"char"      {return Token_CHAR;}
"integer"   {return Token_INTEGER;}
"real"      {return Token_REAL;}
"string"    {return Token_STRING;}

{ws}                {}
{comment}           {}
{identify}          { yylval.iVal = Conversion(yytext, 0); return SKET_ID; }
{integer}           { yylval.iVal = atoi(yytext); return SKET_INTEGER; }
{real}              { yylval.iVal = Conversion(yytext, 1); return SKET_REAL; }
{char_const}        { 
                      char CHaR[2];
                      CHaR[0] = yytext[1];
                      CHaR[1] = 0;
                      yylval.iVal = Conversion(character, 2); 
                      return SKET_CHAR; 
                    }
{string_const}      {
                      char Stri[ID_LEN];
                      int i = strlen(yytext);
                      strcpy(Stri, yytext+1);      
                      Stri[i-2] = 0;
                      yylval.iVal = Conversion(Stri, 4);
                      return SKET_STRING; 
                    }
.                   printf(" {%s %d} Not found the sybmol given.\n", yytext, yytext[0]);
%%


int Conversion(char *Ident, unsigned type = -1) {
    extern SymbolTableNode symboltable[];
    extern int SymbolTableSizeCur;

    //lower symbol

    char IDLOWER[100];
    strcpy(IDLOWER, Ident);
    char *CHR = IDLOWER;

    while(*CHR)
    {
        *CHR = tolower(*CHR);
        CHR++;
    }

    int result = -1;
    int i=0;

    while(i<SymbolTableSizeCur)
    {
        if((strcmp(Ident, symboltable[i].Identify) == 0){
            result = i;
        }
    }


    if (result == -1) {
        strcpy(symboltable[SymbolTableSizeCur].Identify, CHR);      //if can not run, please change the CHR to Ident
        symboltable[SymbolTableSizeCur].Type = type;
        return SymbolTableSizeCur++;
    } else {
        return result;
    }

}



