%{
#include "parser.hpp"
#include "symbol.h"
#include <cctype>
#include <cstdio>
#include <cstring>
extern "C" int yylex();
extern "C" void yyerror(const char *s);
int Conversion(char *identify, unsigned type);
int search(char *identify);
%}

WithPascaldelim   [\t\n\r ]
Number            [0-9]
character         [a-zA-Z]
CONSTOFcharacter  \'.\'
CONSTOFstring     \'.*\'
REALNUM           ({Number}+\.{Number}+)|(\.{Number}+)
INTNUM            {Number}+
IDENTIFY          {character}(_|{character}|{Number})*
segmentation      {WithPascaldelim}+
COMMENTFORPASCL   (\{.*\}|\(\*.*\*\)|\/\/.*)

%%

"("         {return TOKEN_LP;}
")"         {return TOKEN_RP;}
"["         {return TOKEN_LB;}
"]"         {return TOKEN_RB;}
"."         {return TOKEN_DOT;}
","         {return TOKEN_COMMA;}
":"         {return TOKEN_COLON;}
"+"         {return TOKEN_PLUS;}
"-"         {return TOKEN_MINUS;}
"*"         {return TOKEN_MUL;}
"/"         {return TOKEN_DIV;}
">"         {return TOKEN_GT;}
"<"         {return TOKEN_LT;}
"="         {return TOKEN_EQUAL;}
"<>"        {return TOKEN_NE;}
"<="        {return TOKEN_LE;}
">="        {return TOKEN_GE;}
":="        {return TOKEN_ASSIGN;}
"NOT"       {return TOKEN_NOT;}
"MOD"       {return TOKEN_MOD;}
"%"         {return TOKEN_MOD;}
".."        {return TOKEN_DOTDOT;}
";"         {return TOKEN_SEMI;}

"and"       {return TOKEN_AND;}
"array"     {return TOKEN_ARRAY;}
"begin"     {return TOKEN_BEGIN;}
"case"      {return TOKEN_CASE;}
"const"     {return TOKEN_CONST;}
"label"     {return TOKEN_LABEL;}
"do"        {return TOKEN_DO;}
"downto"    {return TOKEN_DOWNTO;}
"else"      {return TOKEN_ELSE;}
"end"       {return TOKEN_END;}
"for"       {return TOKEN_FOR;}
"function"  {return TOKEN_FUNCTION;}
"goto"      {return TOKEN_GOTO;}
"if"        {return TOKEN_IF;}
"in"        {return TOKEN_IN;}
"of"        {return TOKEN_OF;}
"or"        {return TOKEN_OR;}
"packed"    {return TOKEN_PACKED;}
"procedure" {return TOKEN_PROCEDURE;}
"program"   {return TOKEN_PROGRAM;}
"record"    {return TOKEN_RECORD;}
"repeat"    {return TOKEN_REPEAT;}
"set"       {return TOKEN_SET;}
"then"      {return TOKEN_THEN;}
"to"        {return TOKEN_TO;}
"type"      {return TOKEN_TYPE;}
"until"     {return TOKEN_UNTIL;}
"var"       {return TOKEN_VAR;}
"while"     {return TOKEN_WHILE;}
"with"      {return TOKEN_WITH;}

"false"     {return TOKEN_FALSE;}
"true"      {return TOKEN_TRUE;}
"maxint"    {return TOKEN_MAXINT;}
"abs"       {return TOKEN_ABS;}
"chr"       {return TOKEN_CHR;}
"odd"       {return TOKEN_ODD;}
"ord"       {return TOKEN_ORD;}
"pred"      {return TOKEN_PRED;}
"sqr"       {return TOKEN_SQR;}
"sqrt"      {return TOKEN_SQRT;}
"succ"      {return TOKEN_SUCC;}
"write"     {return TOKEN_WRITE;}
"writeln"   {return TOKEN_WRITELN;}
"read"      {return TOKEN_READ;}
"boolean"   {return TOKEN_BOOLEAN;}
"char"      {return TOKEN_CHAR;}
"integer"   {return TOKEN_INTEGER;}
"real"      {return TOKEN_REAL;}
"string"    {return TOKEN_STRING;}


{segmentation}            {}
{COMMENTFORPASCL}         {}
{INTNUM}                  { yylval.iVal = atoi(yytext); return PSACL_int; }
{REALNUM}                 { yylval.iVal = Conversion(yytext, 1); return PSACL_realnum; }
{IDENTIFY}                { yylval.iVal = Conversion(yytext, 0); return PSACL_identify; }
{CONSTOFcharacter}        { 
                            char CHAR[2];
                            CHAR[0] = yytext[1];
                            CHAR[1] = 0;
                            yylval.iVal = Conversion(CHAR, 2); 
                            return PSACL_character;
                        }
{CONSTOFstring}             {
                            char STR[ID_LENTH];
                            strcpy(STR, yytext+1);      
                            int k =strlen(yytext)-2;
                            STR[k] = 0;
                            yylval.iVal = Conversion(STR, 4);
                            return PSACL_str; 
                        }


.                       printf("{%s %d} We do not found such symbol\n", yytext, yytext[0]);
%%



int Conversion(char *identify, unsigned TYPE = -1) {
    extern symbolTableTreeNode PASCALiconTable[];
    extern int PASCLsymboltableSIZECURR;

    // convert to lower case
    char identifyOFLOWER[100];
    strcpy(identifyOFLOWER, identify);

    for (char *t = identifyOFLOWER; *t; t++) {
        *t = tolower(*t);
    }

    int result = -1;

    for (int i = 0; i < PASCLsymboltableSIZECURR; i++) {
        if (strcmp(identify, PASCALiconTable[i].id) == 0) {
            result = i;
        }
    }

    if (result == -1) {
        strcpy(PASCALiconTable[PASCLsymboltableSIZECURR].id, identify);
        PASCALiconTable[PASCLsymboltableSIZECURR].type = TYPE;
        return PASCLsymboltableSIZECURR++;
    } else {
        return result;
    }
}

