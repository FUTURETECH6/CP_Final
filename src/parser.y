%{
#include "symbol.h"
#include "tree.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" int yylex(void);
extern "C" FILE *yyin;

int PASCLsymboltableSIZECURR = 0;
symbolTableTreeNode PASCALiconTable[symbolTable_SIZE];

tree::Program *ast_root;
std::vector<tree::TypeDef *> tmp;
%}

%code requires { 
    #include <iostream>
    #include "tree.h"
    using namespace std;
    using namespace tree;
}

// start symbol
%start PASCAL_PROGRAM

// token
%token 
    TOKEN_LP TOKEN_RP TOKEN_LB TOKEN_RB TOKEN_DOT TOKEN_COMMA TOKEN_COLON TOKEN_PLUS TOKEN_MINUS TOKEN_MUL TOKEN_DIV TOKEN_GT TOKEN_LT TOKEN_EQUAL TOKEN_NE TOKEN_LE TOKEN_GE TOKEN_ASSIGN TOKEN_NOT 
    TOKEN_MOD TOKEN_DOTDOT TOKEN_SEMI TOKEN_AND TOKEN_ARRAY TOKEN_BEGIN TOKEN_CASE TOKEN_CONST TOKEN_LABEL TOKEN_DO TOKEN_DOWNTO TOKEN_ELSE TOKEN_END TOKEN_FOR TOKEN_FUNCTION TOKEN_GOTO 
    TOKEN_IF TOKEN_IN TOKEN_OF TOKEN_OR TOKEN_PACKED TOKEN_PROCEDURE TOKEN_PROGRAM TOKEN_RECORD TOKEN_REPEAT TOKEN_SET TOKEN_THEN TOKEN_TO TOKEN_TYPE TOKEN_UNTIL TOKEN_VAR TOKEN_WHILE TOKEN_WITH 
    TOKEN_ABS TOKEN_CHR TOKEN_ODD TOKEN_ORD TOKEN_PRED TOKEN_SQR TOKEN_SQRT TOKEN_SUCC TOKEN_WRITE TOKEN_WRITELN TOKEN_READ TOKEN_BOOLEAN TOKEN_CHAR TOKEN_INTEGER TOKEN_REAL TOKEN_STRING TOKEN_TRUE TOKEN_FALSE TOKEN_MAXINT

%union {
    int iVal;
    Base* tVal;
    Program* root;
    Define* define;
    Routine* rout;
    Body* body;
    Stm* stm;
    Exp* exp;
    CallExp* callExp;
    tree::Type* type;
    TypeDef* typeDef;
    VarDef* varDef;
    FunctionDef* funcDef;
    std::vector<string> *strVal;
    std::vector<Exp *> *expVal;
    std::vector<tree::Type *> *tyVal;
    std::vector<LabelDef *> *labelVal;
    std::vector<ConstDef *> *constVal;
    std::vector<TypeDef *> *typeVal;
    std::vector<VarDef *> *varVal;
    std::vector<FunctionDef *> *funVal;
    std::vector<Situation *> *situationVal;
}

%token <iVal> PSACL_identify PSACL_int PSACL_realnum PSACL_character PSACL_str 
%type <tVal> EXPR_WITH_PSACL
%type <root> PASCAL_PROGRAM PASCAL_HEAD_PROGRAM
%type <rout> regular_array sub_regular_array
%type <define> regular_array_head
%type <body> regular_array_body statement_mixture statement_array ELSE_PASCAL_ITEM STATEMENT_PASCAL STATEMENT_NON_ICON
%type <stm> STATEMENT_ALLOCATE STATEMENT_procdure STATEMENT_PASCL_IF STATEMENT_PASCL_REPEAT STATEMENT_PASCL_WHILE STATEMENT_PASCL_FOR STATEMENT_PASCL_CASES STATEMENT_PASCL_GT SYSTEM_PASCL_PRODUCRE
%type <exp> VLU_EXP_PSACL_COSNT EXP_SYSTEM_CONTS EXP_PASCL_express EXP_PASCL_exprF EXP_PASCL_TERMF EXP_PASCL_FAT_E
%type <callExp> CALLEXP_SYSTEM_FUNCTION
%type <type> Category_TYPE_DECLARTION Category_TYPE_DECLARTION_EASY Category_TYPE_LIST_DECLARTION Category_REC_DECLARTION Category_STSTEM_TY Category_DIR_PAS
%type <typeDef> Category_Def_PASCAL
%type <funcDef> FUNC_DEF_FIRST_PASCAL_H FUNC_DEF_PROCED_PASCAL_H F_def_PASCAL_DECLARTION Pro_def_PASCAL_DECLARTION FUNC_PARA_PASCL FUNC_PARA_ARRAY_DECLARTION FUNC_PAR_ARRAY_TYPE
%type <expVal> EXPR_ARRAY_VAL EXPR_ARG_VAL_ARRAY
%type <strVal> STR_designate_array STR_PARAMATER_VAR_LIST STR_PARAMATER_VAL_ARRAY
%type <tyVal> TYVAL_FIE_ARRAY_dec TYVAL_FIE_PASCAL_dec
%type <labelVal> ICON_PSACL_ ICON_PSACL_ARRAY
%type <constVal> COSNT_PSACL COSNT_PSACL_ARRAY
%type <typeVal> Category_PSACL Category_PSACL_ARRAY  
%type <varVal> PSACL_VAR_U PSACL_VAR_ARRAY_DE PSACL_VAR_NO_DE 
%type <funVal> informal_PASCAL
%type <situationVal> ARRAY_EXPR_PSACL
%nonassoc "then"
%nonassoc TOKEN_ELSE
%nonassoc TOKEN_PROCEDURE
%nonassoc TOKEN_FUNCTION

%%
PASCAL_PROGRAM:       PASCAL_HEAD_PROGRAM regular_array TOKEN_DOT        {ast_root = $1; ast_root->PASCAL_ADD_TURN($2);}
                    ;
    
PASCAL_HEAD_PROGRAM:  TOKEN_PROGRAM PSACL_identify TOKEN_SEMI             {$$ = new Program(PASCALiconTable[$2].id);}
                    ;

regular_array:              regular_array_head regular_array_body         {$$ = new Routine($1, $2);} // Define, Body
                    ;

sub_regular_array:          regular_array_head regular_array_body         {$$ = new Routine($1, $2);}
                    ;

regular_array_head:         ICON_PSACL_ COSNT_PSACL Category_PSACL PSACL_VAR_U informal_PASCAL     {$$ = new Define(*$1, *$2, *$3, *$4, *$5);} 
                    ;

ICON_PSACL_:           TOKEN_LABEL ICON_PSACL_ARRAY TOKEN_SEMI         {$$ = $2;}
                    | /* empty */                       {$$ = new std::vector<LabelDef*>();} 
                    ;

ICON_PSACL_ARRAY:           ICON_PSACL_ARRAY TOKEN_COMMA PSACL_int      {$$ = $1; $$->push_back(new LabelDef($3));}       
                    | PSACL_int                         {$$ = new std::vector<LabelDef *>(); $$->push_back(new LabelDef($1));}
                    ;

COSNT_PSACL:           TOKEN_CONST COSNT_PSACL_ARRAY           {$$ = $2;}  // std::vector<ConstDef *> *
                    | /* empty */                       {$$ = new std::vector<ConstDef *>();}
                    ;

COSNT_PSACL_ARRAY:      COSNT_PSACL_ARRAY PSACL_identify TOKEN_EQUAL VLU_EXP_PSACL_COSNT TOKEN_SEMI           {$$ = $1; $$->push_back(new ConstDef(PASCALiconTable[$2].id, (Exp*)$4));} // TODO
                    | PSACL_identify TOKEN_EQUAL VLU_EXP_PSACL_COSNT TOKEN_SEMI   {$$ = new std::vector<ConstDef *>(); $$->push_back(new ConstDef(PASCALiconTable[$1].id, (Exp*)$3));}
                    ;

VLU_EXP_PSACL_COSNT:          PSACL_int                         {Value* value= new Value; value->baseType = TY_INTEGER; value->val.integer_value = $1; $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_INTEGER);} // ConstantExp可能有转化问题
                    | PSACL_realnum                            {Value* value= new Value; value->baseType = TY_REAL; value->val.real_value = atof(PASCALiconTable[$1].id); $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_REAL);}
                    | PSACL_character                            {Value* value= new Value; value->baseType = TY_CHAR; value->val.char_value = PASCALiconTable[$1].id[0]; $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_CHAR);}
                    | PSACL_str                          {Value* value= new Value; value->baseType = TY_STRING; value->val.string_value = new string(PASCALiconTable[$1].id); $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_STRING);}
                    | EXP_SYSTEM_CONTS                           {$$ = $1;}
                    ;

EXP_SYSTEM_CONTS:              TOKEN_TRUE                            {Value* value= new Value; value->baseType = TY_BOOLEAN; value->val.boolean_value = true; $$ = new ConstantExp(value); ((ConstantExp*)$$)->return_type = new Type(TY_BOOLEAN);}
                    | TOKEN_FALSE                           {Value* value= new Value; value->baseType = TY_BOOLEAN; value->val.boolean_value = false; $$ = new ConstantExp(value); ((ConstantExp*)$$)->return_type = new Type(TY_BOOLEAN);}
                    | TOKEN_MAXINT                          {Value* value= new Value; value->baseType = TY_INTEGER; value->val.integer_value = 32767; $$ = new ConstantExp(value); ((ConstantExp*)$$)->return_type = new Type(TY_INTEGER);}
                    ;

Category_PSACL:            TOKEN_TYPE Category_PSACL_ARRAY             {$$ = $2; tmp = *$$;}
                    | /* empty */                       {$$ = new std::vector<TypeDef *>();}
                    ;

Category_PSACL_ARRAY:       Category_PSACL_ARRAY Category_Def_PASCAL    {$$ = $1; $$->push_back($2);}
                    | Category_Def_PASCAL                   {$$ = new std::vector<TypeDef *> (); $$->push_back($1);}
                    ;

Category_Def_PASCAL:      PSACL_identify TOKEN_EQUAL Category_TYPE_DECLARTION TOKEN_SEMI     {$$ = new TypeDef(PASCALiconTable[$1].id, $3);} // $3 TypeDef
                    ;

Category_TYPE_DECLARTION:            Category_TYPE_DECLARTION_EASY                  {$$ = $1;} // Type
                    | Category_TYPE_LIST_DECLARTION                   {$$ = $1;}
                    | Category_REC_DECLARTION                  {$$ = $1;}
                    ;

Category_STSTEM_TY:             TOKEN_CHAR                            {$$ = new Type(TY_CHAR);}
                    | TOKEN_INTEGER                         {$$ = new Type(TY_INTEGER);}
                    | TOKEN_REAL                            {$$ = new Type(TY_REAL);}
                    | TOKEN_BOOLEAN                         {$$ = new Type(TY_BOOLEAN);}
                    | TOKEN_STRING                          {$$ = new Type(TY_STRING);}
                    ;

Category_TYPE_DECLARTION_EASY:     Category_STSTEM_TY                          {$$ = $1;} // Type
                    | PSACL_identify                              {bool T1 = false; for(auto tdef : tmp) {if(tdef->name == PASCALiconTable[$1].id) {$$ = tdef->type; T1 = true;}} if(T1==false) yyerror("Semantics Error: Undefined type");} // we define
                    // | TOKEN_LP STR_designate_array TOKEN_RP               {$$ = $2;}
                    | VLU_EXP_PSACL_COSNT TOKEN_DOTDOT VLU_EXP_PSACL_COSNT  {$$ = new Type(TY_ARRAY); $$->array_start = ((ConstantExp*)$1)->value->val.integer_value; 
                                                                                  $$->array_end = ((ConstantExp*)$3)->value->val.integer_value;
                                                        }
                    ;

Category_TYPE_LIST_DECLARTION:      TOKEN_ARRAY TOKEN_LB Category_TYPE_DECLARTION_EASY TOKEN_RB TOKEN_OF Category_TYPE_DECLARTION         {$$ = $3; $$->childType.push_back($6);}
                    ;

Category_REC_DECLARTION:     TOKEN_RECORD TYVAL_FIE_ARRAY_dec TOKEN_END    {$$ = new Type(TY_RECORD); $$->childType = *$2;} // TODO TY_ROCORD type
                    ;

TYVAL_FIE_ARRAY_dec:      TYVAL_FIE_ARRAY_dec TYVAL_FIE_PASCAL_dec        {$$ = $1; $$->insert($$->end(), $2->begin(), $2->end());} // std::vector<Type *>
                    | TYVAL_FIE_PASCAL_dec                        {$$ = new std::vector<Type *>(); $$->insert($$->end(), $1->begin(), $1->end());} 
                    ;

TYVAL_FIE_PASCAL_dec:           STR_designate_array TOKEN_COLON Category_TYPE_DECLARTION TOKEN_SEMI {$$ = new std::vector<Type *>(); for(auto iter : *$1) {Type * tmp = new Type(); tmp->name = iter; tmp->childType.push_back($3); $$->push_back(tmp);}} // std::vector<Type *>
                ;   

STR_designate_array:            STR_designate_array TOKEN_COMMA PSACL_identify            {$$ = $1; $$->push_back(PASCALiconTable[$3].id);}
                    | PSACL_identify                              {$$ = new std::vector<string>(); $$->push_back(PASCALiconTable[$1].id);}
                    ; // TODO std::vector<string>

PSACL_VAR_U:             TOKEN_VAR PSACL_VAR_ARRAY_DE               {$$ = $2;}
                    |/* empty */                        {$$ = new std::vector<VarDef *>();}
                    ;

PSACL_VAR_ARRAY_DE:        PSACL_VAR_ARRAY_DE PSACL_VAR_NO_DE            {$$ = $1; $$->insert($$->end(), $2->begin(), $2->end());}
                    | PSACL_VAR_NO_DE                          {$$ = $1;} // std::vector<VarDef *>
                    ;

PSACL_VAR_NO_DE:             STR_designate_array TOKEN_COLON Category_TYPE_DECLARTION TOKEN_SEMI {$$ = new std::vector<VarDef*>(); for(auto iter : *$1) {$$->push_back(new VarDef(iter, $3));}} // TODO std::vector
                    ;

informal_PASCAL:         informal_PASCAL F_def_PASCAL_DECLARTION        {$$ = $1; $$->push_back($2);} // std::vector <FunctionDef *>
                    | informal_PASCAL Pro_def_PASCAL_DECLARTION       {$$ = $1; $$->push_back($2);}
                    | F_def_PASCAL_DECLARTION                     {$$ = new std::vector<FunctionDef *>; $$->push_back($1);}
                    | Pro_def_PASCAL_DECLARTION                    {$$ = new std::vector<FunctionDef *>; $$->push_back($1);}
                    | %prec "then"                      {$$ = new std::vector<FunctionDef *>();} /* empty */
                    ;

F_def_PASCAL_DECLARTION:        FUNC_DEF_FIRST_PASCAL_H TOKEN_SEMI sub_regular_array TOKEN_SEMI                   {$$ = $1; $$->BODY_CHANGE_PLUS($3->body); $$->DEFINETION_CHANGE_PLUS($3->define);} // FunctionDef *
                    ;

FUNC_DEF_FIRST_PASCAL_H:        TOKEN_FUNCTION PSACL_identify FUNC_PARA_PASCL TOKEN_COLON Category_TYPE_DECLARTION_EASY       {$$ = $3; $$->name = PASCALiconTable[$2].id; $$->setReturnType($5); for(auto argt: $$->args_type) {argt->father = $$;}} // TODO                                                                 
                    ;

Pro_def_PASCAL_DECLARTION:       FUNC_DEF_PROCED_PASCAL_H TOKEN_SEMI sub_regular_array TOKEN_SEMI                  {$$ = $1; $$->BODY_CHANGE_PLUS($3->body); $$->DEFINETION_CHANGE_PLUS($3->define);}
                    ;

FUNC_DEF_PROCED_PASCAL_H:       TOKEN_PROCEDURE PSACL_identify FUNC_PARA_PASCL       {$$ = $3; $$->name = PASCALiconTable[$2].id;} // TODO
                    ;

FUNC_PARA_PASCL:           TOKEN_LP FUNC_PARA_ARRAY_DECLARTION TOKEN_RP          {$$ = $2;}
                    | /* empty */                       {$$ = new FunctionDef("tmp");}
                    ;

FUNC_PARA_ARRAY_DECLARTION:       FUNC_PARA_ARRAY_DECLARTION TOKEN_SEMI FUNC_PAR_ARRAY_TYPE                      {$$ = $1; $$->args_name.insert($$->args_name.end(), $3->args_name.begin(), $3->args_name.end());
                                                                                        $$->args_type.insert($$->args_type.end(), $3->args_type.begin(), $3->args_type.end());
                                                                                        $$->args_is_formal_parameters.insert($$->args_is_formal_parameters.end(), $3->args_is_formal_parameters.begin(), $3->args_is_formal_parameters.end());}
                    | FUNC_PAR_ARRAY_TYPE                                            {$$ = $1;}
                    ;

FUNC_PAR_ARRAY_TYPE:       STR_PARAMATER_VAR_LIST TOKEN_COLON Category_TYPE_DECLARTION_EASY                    {$$ = new FunctionDef("tmp"); for(auto iter : *$1) {$$->ARGS_CHANGE_PLUS(iter, $3, true);}} // true args, type, bool
                    | STR_PARAMATER_VAL_ARRAY TOKEN_COLON Category_TYPE_DECLARTION_EASY                    {$$ = new FunctionDef("tmp"); for(auto iter : *$1) {$$->ARGS_CHANGE_PLUS(iter, $3, false);}} // false
                    ;

STR_PARAMATER_VAR_LIST:        TOKEN_VAR STR_designate_array                   {$$ = $2;} // std::vector <string> procedure true
                    ;

STR_PARAMATER_VAL_ARRAY:        STR_designate_array                         {$$ = $1;} // std::vector <string>
                    ;

regular_array_body:         statement_mixture                     {$$ = $1;} // Body
                    ;

statement_mixture:        TOKEN_BEGIN statement_array TOKEN_END           {$$ = $2;} // Body
                    ;

statement_array:            statement_array STATEMENT_PASCAL TOKEN_SEMI             {$$ = $1; for(auto iter : $2->stms) {$$->addStm(iter);}} // Body
                    | /* empty */                       {$$ = new Body();}
                    ;

STATEMENT_PASCAL:                 PSACL_int TOKEN_COLON STATEMENT_NON_ICON  {$$ = new Body();} // TODO
                    | STATEMENT_NON_ICON                    {$$ = $1;} // Body
                    ;

STATEMENT_NON_ICON:       STATEMENT_ALLOCATE                       {$$ = new Body(); $$->addStm($1);} // Body
                    | STATEMENT_procdure                         {$$ = new Body(); $$->addStm($1);} // Body
                    | statement_mixture                     {$$ = $1;} // Body
                    | STATEMENT_PASCL_IF                           {$$ = new Body(); $$->addStm($1);} // Body
                    | STATEMENT_PASCL_REPEAT                       {$$ = new Body(); $$->addStm($1);} // Body
                    | STATEMENT_PASCL_WHILE                        {$$ = new Body(); $$->addStm($1);} // Body
                    | STATEMENT_PASCL_FOR                          {$$ = new Body(); $$->addStm($1);} // Body
                    | STATEMENT_PASCL_CASES                         {$$ = new Body(); $$->addStm($1);} // Body
                    | STATEMENT_PASCL_GT                         {$$ = new Body(); $$->addStm($1);} // Body
                    ;

STATEMENT_ALLOCATE:          PSACL_identify TOKEN_ASSIGN EXP_PASCL_express          {$$ = new AssignStm(new VariableExp(PASCALiconTable[$1].id), $3);}
                    | PSACL_identify TOKEN_LB EXP_PASCL_express TOKEN_RB TOKEN_ASSIGN EXP_PASCL_express    {$$ = new AssignStm(new BinaryExp(OP_INDEX, new VariableExp(PASCALiconTable[$1].id), $3), $6);}
                    | PSACL_identify TOKEN_DOT PSACL_identify TOKEN_ASSIGN EXP_PASCL_express              {$$ = new AssignStm(new BinaryExp(OP_DOT, new VariableExp(PASCALiconTable[$1].id), new VariableExp(PASCALiconTable[$3].id)), $5);}
                    ;

STATEMENT_procdure:            PSACL_identify                              {$$ = new CallStm(PASCALiconTable[$1].id);}
                    | PSACL_identify TOKEN_LP EXPR_ARG_VAL_ARRAY TOKEN_RP          {$$ = new CallStm(PASCALiconTable[$1].id); for(auto stm : *$3) ((CallStm*)$$)->ARGS_CHANGE_PLUS(stm);} // TODO
                    | SYSTEM_PASCL_PRODUCRE TOKEN_LP EXPR_ARRAY_VAL TOKEN_RP {$$ = $1; for(auto stm : *$3) ((CallStm*)$$)->ARGS_CHANGE_PLUS(stm);}
                    | SYSTEM_PASCL_PRODUCRE TOKEN_LP EXP_PASCL_FAT_E TOKEN_RP         {$$ = $1; ((CallStm*)$$)->ARGS_CHANGE_PLUS($3);}
                    ;

SYSTEM_PASCL_PRODUCRE:             TOKEN_WRITE                           {$$ = new CallStm("write");}
                    | TOKEN_WRITELN                         {$$ = new CallStm("writeln");}
                    | TOKEN_READ                            {$$ = new CallStm("read");}
                    ;

STATEMENT_PASCL_IF:              TOKEN_IF EXP_PASCL_express TOKEN_THEN STATEMENT_PASCAL ELSE_PASCAL_ITEM  {$$ = new IfStm(); ((IfStm*)$$)->setCondition($2); ((IfStm*)$$)->addTrue($4); ((IfStm*)$$)->addFalse($5);}
                    ; // IfStm : Exp Body Body

ELSE_PASCAL_ITEM:          TOKEN_ELSE STATEMENT_PASCAL                       {$$ = $2;} // Body
                    | %prec "then"                      {$$ = new Body();} /* empty */
                    ;

STATEMENT_PASCL_REPEAT:          TOKEN_REPEAT statement_array TOKEN_UNTIL EXP_PASCL_express      {$$ = new RepeatStm(); ((RepeatStm*)$$)->setCondition($4);((RepeatStm*)$$)->addLoop($2);}
                    ;

STATEMENT_PASCL_WHILE:           TOKEN_WHILE EXP_PASCL_express TOKEN_DO STATEMENT_PASCAL      {$$ = new WhileStm($2); ((WhileStm*)$$)->addLoop($4);}
                    ;

STATEMENT_PASCL_FOR:             TOKEN_FOR PSACL_identify TOKEN_ASSIGN EXP_PASCL_express Category_DIR_PAS EXP_PASCL_express TOKEN_DO STATEMENT_PASCAL {$$ = new ForStm(PASCALiconTable[$2].id, $4, $6, $5->baseType); ((ForStm*)$$)->addLoop($8);}
                    ; // iter Exp int Exp Body

Category_DIR_PAS:            TOKEN_TO                              {$$ = new Type(1);}
                    | TOKEN_DOWNTO                          {$$ = new Type(-1);}
                    ;

STATEMENT_PASCL_CASES:            TOKEN_CASE EXP_PASCL_express TOKEN_OF ARRAY_EXPR_PSACL TOKEN_END               {$$ = new CaseStm($2); for(auto situ : *$4) ((CaseStm*)$$)->addSituation(situ);} // TODO
                    ;

ARRAY_EXPR_PSACL:       ARRAY_EXPR_PSACL EXPR_WITH_PSACL          {$$ = $1; ((CaseStm*)$$)->addSituation((Situation*)$2);}
                    | EXPR_WITH_PSACL                         {$$ = new std::vector<Situation *>(); ((CaseStm*)$$)->addSituation((Situation*)$1);}
                    ;

EXPR_WITH_PSACL:            VLU_EXP_PSACL_COSNT TOKEN_COLON STATEMENT_PASCAL TOKEN_SEMI   {$$ = new Situation(); ((Situation*)$$)->addMatch($1); ((Situation*)$$)->addSolution($3);} // Situation
                    | PSACL_identify TOKEN_COLON STATEMENT_PASCAL TOKEN_SEMI          {$$ = new Situation(); ((Situation*)$$)->addMatch(new VariableExp(PASCALiconTable[$1].id)); ((Situation*)$$)->addSolution($3);} //TODO
                    ;

STATEMENT_PASCL_GT:            TOKEN_GOTO PSACL_int                  {$$ = new GotoStm($2);}
                    ;

EXPR_ARRAY_VAL:      EXPR_ARRAY_VAL TOKEN_COMMA EXP_PASCL_express {$$ = $1; $$->push_back($3);} // std::vector<Exp *>
                    | EXP_PASCL_express                        {$$ = new std::vector<Exp *>(); $$->push_back($1);}
                    ;

EXP_PASCL_express:           EXP_PASCL_express TOKEN_GE EXP_PASCL_exprF              {$$ = new BinaryExp(OP_LARGE_EQUAL, $1, $3);}
                    | EXP_PASCL_express TOKEN_GT EXP_PASCL_exprF              {$$ = new BinaryExp(OP_LARGE, $1, $3);}
                    | EXP_PASCL_express TOKEN_LE EXP_PASCL_exprF              {$$ = new BinaryExp(OP_SMALL_EQUAL, $1, $3);}
                    | EXP_PASCL_express TOKEN_LT EXP_PASCL_exprF              {$$ = new BinaryExp(OP_SMALL, $1, $3);}
                    | EXP_PASCL_express TOKEN_EQUAL EXP_PASCL_exprF           {$$ = new BinaryExp(OP_EQUAL, $1, $3);}
                    | EXP_PASCL_express TOKEN_NE EXP_PASCL_exprF              {$$ = new BinaryExp(OP_NOT_EQUAL, $1, $3);}
                    | EXP_PASCL_exprF                              {$$ = $1;}
                    ;

EXP_PASCL_exprF:                 EXP_PASCL_exprF TOKEN_PLUS EXP_PASCL_TERMF                  {$$ = new BinaryExp(OP_ADD, $1, $3);}
                    | EXP_PASCL_exprF TOKEN_MINUS EXP_PASCL_TERMF                 {$$ = new BinaryExp(OP_MINUS, $1, $3);}
                    | EXP_PASCL_exprF TOKEN_OR EXP_PASCL_TERMF                    {$$ = new BinaryExp(OP_OR, $1, $3);}
                    | EXP_PASCL_TERMF                              {$$ = $1;}
                    ;

EXP_PASCL_TERMF:                 EXP_PASCL_TERMF TOKEN_MUL EXP_PASCL_FAT_E                 {$$ = new BinaryExp(OP_MULTI, $1, $3);}
                    | EXP_PASCL_TERMF TOKEN_DIV EXP_PASCL_FAT_E                 {$$ = new BinaryExp(OP_RDIV, $1, $3);}
                    | EXP_PASCL_TERMF TOKEN_MOD EXP_PASCL_FAT_E                 {$$ = new BinaryExp(OP_MOD, $1, $3);}
                    | EXP_PASCL_TERMF TOKEN_AND EXP_PASCL_FAT_E                 {$$ = new BinaryExp(OP_AND, $1, $3);}
                    | EXP_PASCL_FAT_E                            {$$ = $1;}
                    ;

CALLEXP_SYSTEM_FUNCTION:            TOKEN_ABS                             {$$ = new CallExp("TOKEN_ABS");}
                    | TOKEN_CHR                             {$$ = new CallExp("TOKEN_CHR");}
                    | TOKEN_ODD                             {$$ = new CallExp("TOKEN_ODD");}
                    | TOKEN_ORD                             {$$ = new CallExp("TOKEN_ORD");}
                    | TOKEN_PRED                            {$$ = new CallExp("TOKEN_PRED");}
                    | TOKEN_SQR                             {$$ = new CallExp("TOKEN_SQR");}
                    | TOKEN_SQRT                            {$$ = new CallExp("TOKEN_SQRT");}
                    | TOKEN_SUCC                            {$$ = new CallExp("TOKEN_SUCC");}
                    ;


EXP_PASCL_FAT_E:               PSACL_identify                              {$$ = new VariableExp(PASCALiconTable[$1].id);}
                    | PSACL_identify TOKEN_LP EXPR_ARG_VAL_ARRAY TOKEN_RP          {$$ = new CallExp(PASCALiconTable[$1].id); for(auto stm : *$3) ((CallExp*)$$)->ARGS_CHANGE_PLUS(stm);} // EXPR_ARG_VAL_ARRAY is a std::vector<Exp*>
                    | CALLEXP_SYSTEM_FUNCTION                         {$$ = $1;}
                    | CALLEXP_SYSTEM_FUNCTION TOKEN_LP EXPR_ARG_VAL_ARRAY TOKEN_RP     {$$ = $1; for(auto stm : *$3) ((CallExp*)$$)->ARGS_CHANGE_PLUS(stm);} // EXPR_ARG_VAL_ARRAY is a std::vector<Exp*>
                    | VLU_EXP_PSACL_COSNT                       {$$ = $1;}
                    | TOKEN_LP EXP_PASCL_express TOKEN_RP              {$$ = $2;}
                    | TOKEN_NOT EXP_PASCL_FAT_E                      {$$ = new UnaryExp(OP_NOT, $2);}
                    | TOKEN_MINUS EXP_PASCL_FAT_E                    {$$ = new UnaryExp(OP_MINUS, $2);}
                    | PSACL_identify TOKEN_LB EXP_PASCL_express TOKEN_RB         {$$ = new BinaryExp(OP_INDEX, new VariableExp(PASCALiconTable[$1].id), $3);}
                    | PSACL_identify TOKEN_DOT PSACL_identify                   {$$ = new BinaryExp(OP_DOT, new VariableExp(PASCALiconTable[$1].id), new VariableExp(PASCALiconTable[$3].id));}
                    ;

EXPR_ARG_VAL_ARRAY:            EXPR_ARG_VAL_ARRAY TOKEN_COMMA EXP_PASCL_express      {$$ = $1; $$->push_back($3);} // std::vector<Exp*>
                    | EXP_PASCL_express                        {$$ = new std::vector<Exp *>; $$->push_back($1);}
                    ;

%%

int doyyparse(char *NAMEFILE) {
    FILE *fp;

    if ((fp = fopen(NAMEFILE, "r")) != NULL)
        yyin = fp;
    else
        return -1;

    do {
        yyparse();
    } while (!feof(yyin));

    return 0;
}
