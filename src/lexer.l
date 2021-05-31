%{
#include "parser.hpp"
#include "symbol.h"
#include <cctype>
#include <cstdio>
#include <cstring>
extern "C" int yylex();
extern "C" void yyerror(const char *s);
int transform(char *id, unsigned type);
int search(char *id);
%}

delimiter       [\t\n\r ]
digit           [0-9]
char            [a-zA-Z]
char_const      \'.\'
string_const    \'.*\'
real            ({digit}+\.{digit}+)|(\.{digit}+)
integer         {digit}+
id              {char}(_|{char}|{digit})*
ws              {delimiter}+
comment         (\{.*\}|\(\*.*\*\)|\/\/.*)

%%

"("         {return T_LP;}
")"         {return T_RP;}
"["         {return T_LB;}
"]"         {return T_RB;}
"."         {return T_DOT;}
","         {return T_COMMA;}
":"         {return T_COLON;}
"+"         {return T_PLUS;}
"-"         {return T_MINUS;}
"*"         {return T_MUL;}
"/"         {return T_DIV;}
">"         {return T_GT;}
"<"         {return T_LT;}
"="         {return T_EQUAL;}
"<>"        {return T_NE;}
"<="        {return T_LE;}
">="        {return T_GE;}
":="        {return T_ASSIGN;}
"NOT"       {return T_NOT;}
"MOD"       {return T_MOD;}
"%"         {return T_MOD;}
".."        {return T_DOTDOT;}
";"         {return T_SEMI;}

"and"       {return T_AND;}
"array"     {return T_ARRAY;}
"begin"     {return T_BEGIN;}
"case"      {return T_CASE;}
"const"     {return T_CONST;}
"label"     {return T_LABEL;}
"do"        {return T_DO;}
"downto"    {return T_DOWNTO;}
"else"      {return T_ELSE;}
"end"       {return T_END;}
"for"       {return T_FOR;}
"function"  {return T_FUNCTION;}
"goto"      {return T_GOTO;}
"if"        {return T_IF;}
"in"        {return T_IN;}
"of"        {return T_OF;}
"or"        {return T_OR;}
"packed"    {return T_PACKED;}
"procedure" {return T_PROCEDURE;}
"program"   {return T_PROGRAM;}
"record"    {return T_RECORD;}
"repeat"    {return T_REPEAT;}
"set"       {return T_SET;}
"then"      {return T_THEN;}
"to"        {return T_TO;}
"type"      {return T_TYPE;}
"until"     {return T_UNTIL;}
"var"       {return T_VAR;}
"while"     {return T_WHILE;}
"with"      {return T_WITH;}

"false"     {return T_FALSE;}
"true"      {return T_TRUE;}
"maxint"    {return T_MAXINT;}
"abs"       {return T_ABS;}
"chr"       {return T_CHR;}
"odd"       {return T_ODD;}
"ord"       {return T_ORD;}
"pred"      {return T_PRED;}
"sqr"       {return T_SQR;}
"sqrt"      {return T_SQRT;}
"succ"      {return T_SUCC;}
"write"     {return T_WRITE;}
"writeln"   {return T_WRITELN;}
"read"      {return T_READ;}
"boolean"   {return T_BOOLEAN;}
"char"      {return T_CHAR;}
"integer"   {return T_INTEGER;}
"real"      {return T_REAL;}
"string"    {return T_STRING;}

{id}                { yylval.iVal = transform(yytext, 0); return S_ID; }
{char_const}        { 
                      char character[2];
                      character[0] = yytext[1];
                      character[1] = 0;
                      yylval.iVal = transform(character, 2); 
                      return S_CHAR; 
                    }
{string_const}      {
                      char string[ID_MAX_LEN];
                      strcpy(string, yytext+1);      
                      string[strlen(yytext)-2] = 0;
                      yylval.iVal = transform(string, 4);
                      return S_STRING; 
                    }
{integer}           { yylval.iVal = atoi(yytext); return S_INTEGER; }
{real}              { yylval.iVal = transform(yytext, 1); return S_REAL; }
{ws}                {}
{comment}           {}
.                   printf("Unexpected symbol found. %s %d\n", yytext, yytext[0]);
%%

int transform(char *id, unsigned type = -1) {
    extern symTabNode symtab[];
    extern int currentSymTabSize;

    // convert to lower case
    char lower_id[100];
    strcpy(lower_id, id);
    for (char *p = lower_id; *p; p++) {
        *p = tolower(*p);
    }
    int result = search(id);

    if (result == -1) {
        strcpy(symtab[currentSymTabSize].id, id);
        symtab[currentSymTabSize].type = type;
        return currentSymTabSize++;
    } else {
        return result;
    }
}
int search(char *id) {
    extern symTabNode symtab[];
    extern int currentSymTabSize;

    // convert to lower case
    char lower_id[100];
    strcpy(lower_id, id);
    for (char *p = lower_id; *p; p++) {
        *p = tolower(*p);
    }

    for (int i = 0; i < currentSymTabSize; i++) {
        if (strcmp(id, symtab[i].id) == 0) {
            return i;
        }
    }
    return -1;
}
