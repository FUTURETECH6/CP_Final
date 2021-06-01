%{
#include "symbol.h"
#include "tree.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" int yylex(void);
extern "C" FILE *yyin;

int SymbolTableSizeCur = 0;
SymbolTableNode symboltable[SYMTAB_SIZE];

tree::Program *Root_AST;
std::vector<tree::TypeDef *> tmp;
%}

%code requires {
    #include <iostream>
    #include "tree.h"
    using namespace std;
    using namespace tree;
}

// start symbol
%start program

// token
%token 
    Token_LeftP Token_RightP Token_LeftB Token_RightB Token_DOT Token_COMMA Token_COLON Token_PLUS Token_MINUS Token_MUL Token_DIV Token_GT Token_LT Token_EQUAL Token_NE Token_LE Token_GE Token_ASSIGN Token_NOT 
    Token_MOD Token_DOTDOT Token_SEMI Token_AND Token_ARRAY Token_BEGIN Token_CASE Token_CONST Token_LABEL Token_DO Token_DOWNTO Token_ELSE Token_END Token_FOR Token_FUNCTION Token_GOTO 
    Token_IF Token_IN Token_OF Token_OR Token_PACKED Token_PROCEDURE Token_PROGRAM Token_RECORD Token_REPEAT Token_SET Token_THEN Token_TO Token_TYPE Token_UNTIL Token_VAR Token_WHILE Token_WITH 
    Token_ABS Token_CHR Token_ODD Token_ORD Token_PRED Token_SQR Token_SQRT Token_SUCC Token_WRITE Token_WRITELN Token_READ Token_BOOLEAN Token_CHAR Token_INTEGER Token_REAL Token_STRING Token_TRUE Token_FALSE Token_MAXINT

%union {
    int iVal;
    Base* tVal;
    Program* Root;
    Define* define;
    Routine* RoutineT;
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


%token <iVal> SKET_ID SKET_INTEGER SKET_REAL SKET_CHAR SKET_STRING 
%type <tVal> case_expr
%type <Root> program program_head
%type <RoutineT> routine sub_routine
%type <define> routine_head
%type <body> routine_body compound_stmt stmt_list else_clause stmt non_label_stmt
%type <stm> assign_stmt proc_stmt if_stmt repeat_stmt while_stmt for_stmt case_stmt goto_stmt sys_proc
%type <exp> const_value sys_con expression expr term factor
%type <callExp> sys_funct
%type <type> type_decl simple_type_decl array_type_decl record_type_decl sys_type direction
%type <typeDef> type_definition
%type <funcDef> function_head procedure_head function_decl procedure_decl parameters para_decl_list para_type_list
%type <expVal> expression_list args_list
%type <strVal> name_list var_para_list val_para_list
%type <tyVal> field_decl_list field_decl
%type <labelVal> label_part label_list
%type <constVal> const_part const_expr_list
%type <typeVal> type_part type_decl_list  
%type <varVal> var_part var_decl_list var_decl 
%type <funVal> routine_part
%type <situationVal> case_expr_list

%nonassoc "then"
%nonassoc Token_ELSE
%nonassoc Token_PROCEDURE
%nonassoc Token_FUNCTION

%%
program:              program_head routine Token_DOT        {Root_AST = $1; Root_AST->addRoutine($2);}
                    ;
    
program_head:         Token_PROGRAM SKET_ID Token_SEMI             {$$ = new Program(symboltable[$2].Identify);}
                    ;

routine:              routine_head routine_body         {$$ = new Routine($1, $2);} // Define, Body
                    ;

sub_routine:          routine_head routine_body         {$$ = new Routine($1, $2);}
                    ;

routine_head:         label_part const_part type_part var_part routine_part     {$$ = new Define(*$1, *$2, *$3, *$4, *$5);} 
                    ;

label_part:           Token_LABEL label_list Token_SEMI         {$$ = $2;}
                    | /* empty */                       {$$ = new std::vector<LabelDef*>();} 
                    ;

label_list:           label_list Token_COMMA SKET_INTEGER      {$$ = $1; $$->push_back(new LabelDef($3));}       
                    | SKET_INTEGER                         {$$ = new std::vector<LabelDef *>(); $$->push_back(new LabelDef($1));}
                    ;

const_part:           Token_CONST const_expr_list           {$$ = $2;}  // std::vector<ConstDef *> *
                    | /* empty */                       {$$ = new std::vector<ConstDef *>();}
                    ;

const_expr_list:      const_expr_list SKET_ID Token_EQUAL const_value Token_SEMI           {$$ = $1; $$->push_back(new ConstDef(symboltable[$2].Identify, (Exp*)$4));} // TODO
                    | SKET_ID Token_EQUAL const_value Token_SEMI   {$$ = new std::vector<ConstDef *>(); $$->push_back(new ConstDef(symboltable[$1].Identify, (Exp*)$3));}
                    ;

const_value:          SKET_INTEGER                         {Value* value= new Value; value->base_type = TY_INTEGER; value->val.integer_value = $1; $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_INTEGER);} // ConstantExp可能有转化问题
                    | SKET_REAL                            {Value* value= new Value; value->base_type = TY_REAL; value->val.real_value = atof(symboltable[$1].Identify); $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_REAL);}
                    | SKET_CHAR                            {Value* value= new Value; value->base_type = TY_CHAR; value->val.char_value = symboltable[$1].Identify[0]; $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_CHAR);}
                    | SKET_STRING                          {Value* value= new Value; value->base_type = TY_STRING; value->val.string_value = new string(symboltable[$1].Identify); $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_STRING);}
                    | sys_con                           {$$ = $1;}
                    ;

sys_con:              Token_TRUE                            {Value* value= new Value; value->base_type = TY_BOOLEAN; value->val.boolean_value = true; $$ = new ConstantExp(value); ((ConstantExp*)$$)->return_type = new Type(TY_BOOLEAN);}
                    | Token_FALSE                           {Value* value= new Value; value->base_type = TY_BOOLEAN; value->val.boolean_value = false; $$ = new ConstantExp(value); ((ConstantExp*)$$)->return_type = new Type(TY_BOOLEAN);}
                    | Token_MAXINT                          {Value* value= new Value; value->base_type = TY_INTEGER; value->val.integer_value = 32767; $$ = new ConstantExp(value); ((ConstantExp*)$$)->return_type = new Type(TY_INTEGER);}
                    ;

type_part:            Token_TYPE type_decl_list             {$$ = $2; tmp = *$$;}
                    | /* empty */                       {$$ = new std::vector<TypeDef *>();}
                    ;

type_decl_list:       type_decl_list type_definition    {$$ = $1; $$->push_back($2);}
                    | type_definition                   {$$ = new std::vector<TypeDef *> (); $$->push_back($1);}
                    ;

type_definition:      SKET_ID Token_EQUAL type_decl Token_SEMI     {$$ = new TypeDef(symboltable[$1].Identify, $3);} // $3 TypeDef
                    ;

type_decl:            simple_type_decl                  {$$ = $1;} // Type
                    | array_type_decl                   {$$ = $1;}
                    | record_type_decl                  {$$ = $1;}
                    ;

sys_type:             Token_CHAR                            {$$ = new Type(TY_CHAR);}
                    | Token_INTEGER                         {$$ = new Type(TY_INTEGER);}
                    | Token_REAL                            {$$ = new Type(TY_REAL);}
                    | Token_BOOLEAN                         {$$ = new Type(TY_BOOLEAN);}
                    | Token_STRING                          {$$ = new Type(TY_STRING);}
                    ;

simple_type_decl:     sys_type                          {$$ = $1;} // Type
                    | SKET_ID                              {bool flag = false; for(auto tdef : tmp) {if(tdef->name == symboltable[$1].Identify) {$$ = tdef->type; flag = true;}} if(flag==false) yyerror("Semantics Error: Undefined type");} // we define
                    // | Token_LeftP name_list Token_RightP               {$$ = $2;}
                    | const_value Token_DOTDOT const_value  {$$ = new Type(TY_ARRAY); $$->array_start = ((ConstantExp*)$1)->value->val.integer_value; 
                                                                                  $$->array_end = ((ConstantExp*)$3)->value->val.integer_value;
                                                        }
                    ;

array_type_decl:      Token_ARRAY Token_LeftB simple_type_decl Token_RightB Token_OF type_decl         {$$ = $3; $$->child_type.push_back($6);}
                    ;

record_type_decl:     Token_RECORD field_decl_list Token_END    {$$ = new Type(TY_RECORD); $$->child_type = *$2;} // TODO TY_ROCORD type
                    ;

field_decl_list:      field_decl_list field_decl        {$$ = $1; $$->insert($$->end(), $2->begin(), $2->end());} // std::vector<Type *>
                    | field_decl                        {$$ = new std::vector<Type *>(); $$->insert($$->end(), $1->begin(), $1->end());} 
                    ;

field_decl:           name_list Token_COLON type_decl Token_SEMI {$$ = new std::vector<Type *>(); for(auto iter : *$1) {Type * tmp = new Type(); tmp->name = iter; tmp->child_type.push_back($3); $$->push_back(tmp);}} // std::vector<Type *>
                ;   

name_list:            name_list Token_COMMA SKET_ID            {$$ = $1; $$->push_back(symboltable[$3].Identify);}
                    | SKET_ID                              {$$ = new std::vector<string>(); $$->push_back(symboltable[$1].Identify);}
                    ; // TODO std::vector<string>

var_part:             Token_VAR var_decl_list               {$$ = $2;}
                    |/* empty */                        {$$ = new std::vector<VarDef *>();}
                    ;

var_decl_list:        var_decl_list var_decl            {$$ = $1; $$->insert($$->end(), $2->begin(), $2->end());}
                    | var_decl                          {$$ = $1;} // std::vector<VarDef *>
                    ;

var_decl:             name_list Token_COLON type_decl Token_SEMI {$$ = new std::vector<VarDef*>(); for(auto iter : *$1) {$$->push_back(new VarDef(iter, $3));}} // TODO std::vector
                    ;

routine_part:         routine_part function_decl        {$$ = $1; $$->push_back($2);} // std::vector <FunctionDef *>
                    | routine_part procedure_decl       {$$ = $1; $$->push_back($2);}
                    | function_decl                     {$$ = new std::vector<FunctionDef *>; $$->push_back($1);}
                    | procedure_decl                    {$$ = new std::vector<FunctionDef *>; $$->push_back($1);}
                    | %prec "then"                      {$$ = new std::vector<FunctionDef *>();} /* empty */
                    ;

function_decl:        function_head Token_SEMI sub_routine Token_SEMI                   {$$ = $1; $$->addBody($3->body); $$->addDefine($3->define);} // FunctionDef *
                    ;

function_head:        Token_FUNCTION SKET_ID parameters Token_COLON simple_type_decl       {$$ = $3; $$->name = symboltable[$2].Identify; $$->setReturnType($5); for(auto argt: $$->args_type) {argt->father = $$;}} // TODO                                                                 
                    ;

procedure_decl:       procedure_head Token_SEMI sub_routine Token_SEMI                  {$$ = $1; $$->addBody($3->body); $$->addDefine($3->define);}
                    ;

procedure_head:       Token_PROCEDURE SKET_ID parameters       {$$ = $3; $$->name = symboltable[$2].Identify;} // TODO
                    ;

parameters:           Token_LeftP para_decl_list Token_RightP          {$$ = $2;}
                    | /* empty */                       {$$ = new FunctionDef("tmp");}
                    ;

para_decl_list:       para_decl_list Token_SEMI para_type_list                      {$$ = $1; $$->args_name.insert($$->args_name.end(), $3->args_name.begin(), $3->args_name.end());
                                                                                        $$->args_type.insert($$->args_type.end(), $3->args_type.begin(), $3->args_type.end());
                                                                                        $$->args_is_formal_parameters.insert($$->args_is_formal_parameters.end(), $3->args_is_formal_parameters.begin(), $3->args_is_formal_parameters.end());}
                    | para_type_list                                            {$$ = $1;}
                    ;

para_type_list:       var_para_list Token_COLON simple_type_decl                    {$$ = new FunctionDef("tmp"); for(auto iter : *$1) {$$->addArgs(iter, $3, true);}} // true args, type, bool
                    | val_para_list Token_COLON simple_type_decl                    {$$ = new FunctionDef("tmp"); for(auto iter : *$1) {$$->addArgs(iter, $3, false);}} // false
                    ;

var_para_list:        Token_VAR name_list                   {$$ = $2;} // std::vector <string> procedure true
                    ;

val_para_list:        name_list                         {$$ = $1;} // std::vector <string>
                    ;

routine_body:         compound_stmt                     {$$ = $1;} // Body
                    ;

compound_stmt:        Token_BEGIN stmt_list Token_END           {$$ = $2;} // Body
                    ;

stmt_list:            stmt_list stmt Token_SEMI             {$$ = $1; for(auto iter : $2->stms) {$$->addStm(iter);}} // Body
                    | /* empty */                       {$$ = new Body();}
                    ;

stmt:                 SKET_INTEGER Token_COLON non_label_stmt  {$$ = new Body();} // TODO
                    | non_label_stmt                    {$$ = $1;} // Body
                    ;

non_label_stmt:       assign_stmt                       {$$ = new Body(); $$->addStm($1);} // Body
                    | proc_stmt                         {$$ = new Body(); $$->addStm($1);} // Body
                    | compound_stmt                     {$$ = $1;} // Body
                    | if_stmt                           {$$ = new Body(); $$->addStm($1);} // Body
                    | repeat_stmt                       {$$ = new Body(); $$->addStm($1);} // Body
                    | while_stmt                        {$$ = new Body(); $$->addStm($1);} // Body
                    | for_stmt                          {$$ = new Body(); $$->addStm($1);} // Body
                    | case_stmt                         {$$ = new Body(); $$->addStm($1);} // Body
                    | goto_stmt                         {$$ = new Body(); $$->addStm($1);} // Body
                    ;

assign_stmt:          SKET_ID Token_ASSIGN expression          {$$ = new AssignStm(new VariableExp(symboltable[$1].Identify), $3);}
                    | SKET_ID Token_LeftB expression Token_RightB Token_ASSIGN expression    {$$ = new AssignStm(new BinaryExp(OP_INDEX, new VariableExp(symboltable[$1].Identify), $3), $6);}
                    | SKET_ID Token_DOT SKET_ID Token_ASSIGN expression              {$$ = new AssignStm(new BinaryExp(OP_DOT, new VariableExp(symboltable[$1].Identify), new VariableExp(symboltable[$3].Identify)), $5);}
                    ;

proc_stmt:            SKET_ID                              {$$ = new CallStm(symboltable[$1].Identify);}
                    | SKET_ID Token_LeftP args_list Token_RightP          {$$ = new CallStm(symboltable[$1].Identify); for(auto stm : *$3) ((CallStm*)$$)->addArgs(stm);} // TODO
                    | sys_proc Token_LeftP expression_list Token_RightP {$$ = $1; for(auto stm : *$3) ((CallStm*)$$)->addArgs(stm);}
                    | sys_proc Token_LeftP factor Token_RightP         {$$ = $1; ((CallStm*)$$)->addArgs($3);}
                    ;

sys_proc:             Token_WRITE                           {$$ = new CallStm("write");}
                    | Token_WRITELN                         {$$ = new CallStm("writeln");}
                    | Token_READ                            {$$ = new CallStm("read");}
                    ;

if_stmt:              Token_IF expression Token_THEN stmt else_clause  {$$ = new IfStm(); ((IfStm*)$$)->setCondition($2); ((IfStm*)$$)->addTrue($4); ((IfStm*)$$)->addFalse($5);}
                    ; // IfStm : Exp Body Body

else_clause:          Token_ELSE stmt                       {$$ = $2;} // Body
                    | %prec "then"                      {$$ = new Body();} /* empty */
                    ;

repeat_stmt:          Token_REPEAT stmt_list Token_UNTIL expression      {$$ = new RepeatStm(); ((RepeatStm*)$$)->setCondition($4);((RepeatStm*)$$)->addLoop($2);}
                    ;

while_stmt:           Token_WHILE expression Token_DO stmt      {$$ = new WhileStm($2); ((WhileStm*)$$)->addLoop($4);}
                    ;

for_stmt:             Token_FOR SKET_ID Token_ASSIGN expression direction expression Token_DO stmt {$$ = new ForStm(symboltable[$2].Identify, $4, $6, $5->base_type); ((ForStm*)$$)->addLoop($8);}
                    ; // iter Exp int Exp Body

direction:            Token_TO                              {$$ = new Type(1);}
                    | Token_DOWNTO                          {$$ = new Type(-1);}
                    ;

case_stmt:            Token_CASE expression Token_OF case_expr_list Token_END               {$$ = new CaseStm($2); for(auto situ : *$4) ((CaseStm*)$$)->addSituation(situ);} // TODO
                    ;

case_expr_list:       case_expr_list case_expr          {$$ = $1; ((CaseStm*)$$)->addSituation((Situation*)$2);}
                    | case_expr                         {$$ = new std::vector<Situation *>(); ((CaseStm*)$$)->addSituation((Situation*)$1);}
                    ;

case_expr:            const_value Token_COLON stmt Token_SEMI   {$$ = new Situation(); ((Situation*)$$)->addMatch($1); ((Situation*)$$)->addSolution($3);} // Situation
                    | SKET_ID Token_COLON stmt Token_SEMI          {$$ = new Situation(); ((Situation*)$$)->addMatch(new VariableExp(symboltable[$1].Identify)); ((Situation*)$$)->addSolution($3);} //TODO
                    ;

goto_stmt:            Token_GOTO SKET_INTEGER                  {$$ = new GotoStm($2);}
                    ;

expression_list:      expression_list Token_COMMA expression {$$ = $1; $$->push_back($3);} // std::vector<Exp *>
                    | expression                        {$$ = new std::vector<Exp *>(); $$->push_back($1);}
                    ;

expression:           expression Token_GE expr              {$$ = new BinaryExp(OP_LARGE_EQUAL, $1, $3);}
                    | expression Token_GT expr              {$$ = new BinaryExp(OP_LARGE, $1, $3);}
                    | expression Token_LE expr              {$$ = new BinaryExp(OP_SMALL_EQUAL, $1, $3);}
                    | expression Token_LT expr              {$$ = new BinaryExp(OP_SMALL, $1, $3);}
                    | expression Token_EQUAL expr           {$$ = new BinaryExp(OP_EQUAL, $1, $3);}
                    | expression Token_NE expr              {$$ = new BinaryExp(OP_NOT_EQUAL, $1, $3);}
                    | expr                              {$$ = $1;}
                    ;

expr:                 expr Token_PLUS term                  {$$ = new BinaryExp(OP_ADD, $1, $3);}
                    | expr Token_MINUS term                 {$$ = new BinaryExp(OP_MINUS, $1, $3);}
                    | expr Token_OR term                    {$$ = new BinaryExp(OP_OR, $1, $3);}
                    | term                              {$$ = $1;}
                    ;

term:                 term Token_MUL factor                 {$$ = new BinaryExp(OP_MULTI, $1, $3);}
                    | term Token_DIV factor                 {$$ = new BinaryExp(OP_RDIV, $1, $3);}
                    | term Token_MOD factor                 {$$ = new BinaryExp(OP_MOD, $1, $3);}
                    | term Token_AND factor                 {$$ = new BinaryExp(OP_AND, $1, $3);}
                    | factor                            {$$ = $1;}
                    ;

sys_funct:            Token_ABS                             {$$ = new CallExp("Token_ABS");}
                    | Token_CHR                             {$$ = new CallExp("Token_CHR");}
                    | Token_ODD                             {$$ = new CallExp("Token_ODD");}
                    | Token_ORD                             {$$ = new CallExp("Token_ORD");}
                    | Token_PRED                            {$$ = new CallExp("Token_PRED");}
                    | Token_SQR                             {$$ = new CallExp("Token_SQR");}
                    | Token_SQRT                            {$$ = new CallExp("Token_SQRT");}
                    | Token_SUCC                            {$$ = new CallExp("Token_SUCC");}
                    ;


factor:               SKET_ID                              {$$ = new VariableExp(symboltable[$1].Identify);}
                    | SKET_ID Token_LeftP args_list Token_RightP          {$$ = new CallExp(symboltable[$1].Identify); for(auto stm : *$3) ((CallExp*)$$)->addArgs(stm);} // args_list is a std::vector<Exp*>
                    | sys_funct                         {$$ = $1;}
                    | sys_funct Token_LeftP args_list Token_RightP     {$$ = $1; for(auto stm : *$3) ((CallExp*)$$)->addArgs(stm);} // args_list is a std::vector<Exp*>
                    | const_value                       {$$ = $1;}
                    | Token_LeftP expression Token_RightP              {$$ = $2;}
                    | Token_NOT factor                      {$$ = new UnaryExp(OP_NOT, $2);}
                    | Token_MINUS factor                    {$$ = new UnaryExp(OP_MINUS, $2);}
                    | SKET_ID Token_LeftB expression Token_RightB         {$$ = new BinaryExp(OP_INDEX, new VariableExp(symboltable[$1].Identify), $3);}
                    | SKET_ID Token_DOT SKET_ID                   {$$ = new BinaryExp(OP_DOT, new VariableExp(symboltable[$1].Identify), new VariableExp(symboltable[$3].Identify));}
                    ;

args_list:            args_list Token_COMMA expression      {$$ = $1; $$->push_back($3);} // std::vector<Exp*>
                    | expression                        {$$ = new std::vector<Exp *>; $$->push_back($1);}
                    ;

%%

int doyyparse(char *file) {
    FILE *fp;
    if ((fp = fopen(file, "r")) != NULL)
        yyin = fp;
    else
        return -1;

    do {
        yyparse();
    } while (!feof(yyin));

    return 0;
}
