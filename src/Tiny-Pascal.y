%{
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "util.h"
#include "ast.h"

extern "C" int yylex(void);
extern "C" FILE* yyin;


int currentSymTabSize = 0;
sym_tab_node symtab[SYM_TAB_LEN];

ast::Program* ast_root;
std::vector<ast::TypeDef *> tmp;
%}

%code requires {
    #include <iostream>
    #include "ast.h"
    using namespace std;
    using namespace ast;

}

// start symbol
%start program

// token
%token 
    T_LP T_RP T_LB T_RB T_DOT T_COMMA T_COLON T_PLUS T_MINUS T_MUL T_DIV T_GT T_LT T_EQUAL T_NE T_LE T_GE T_ASSIGN T_NOT 
    T_MOD T_DOTDOT T_SEMI T_AND T_ARRAY T_BEGIN T_CASE T_CONST T_LABEL T_DO T_DOWNTO T_ELSE T_END T_FOR T_FUNCTION T_GOTO 
    T_IF T_IN T_OF T_OR T_PACKED T_PROCEDURE T_PROGRAM T_RECORD T_REPEAT T_SET T_THEN T_TO T_TYPE T_UNTIL T_VAR T_WHILE T_WITH 
    T_ABS T_CHR T_ODD T_ORD T_PRED T_SQR T_SQRT T_SUCC T_WRITE T_WRITELN T_READ T_BOOLEAN T_CHAR T_INTEGER T_REAL T_STRING T_TRUE T_FALSE T_MAXINT

%union {
    int iVal;
    Base* tVal;
    Program* root;
    Define* define;
    Routine* rout;
    Body* body;
    Stm* stm;
    Exp* exp;
    CallExp* callexp;
    ast::Type* type;
    TypeDef* tydef;
    VarDef* vardef;
    FunctionDef* functiondef;
    std::vector<string> *str;
    std::vector<Exp *> *expVal;
    std::vector<ast::Type *> *tyVal;
    std::vector<LabelDef *> *labelVal;
    std::vector<ConstDef *> *constVal;
    std::vector<TypeDef *> *typeVal;
    std::vector<VarDef *> *varVal;
    std::vector<FunctionDef *> *funVal;
    std::vector<Situation *> *situationVal;
}

%token <iVal> S_ID S_INTEGER S_REAL S_CHAR S_STRING 
%type <tVal> case_expr
%type <root> program program_head
%type <rout> routine sub_routine
%type <define> routine_head
%type <body> routine_body compound_stmt stmt_list else_clause stmt non_label_stmt
%type <stm> assign_stmt proc_stmt if_stmt repeat_stmt while_stmt for_stmt case_stmt goto_stmt sys_proc
%type <exp> const_value sys_con expression expr term factor
%type <callexp> sys_funct
%type <type> type_decl simple_type_decl array_type_decl record_type_decl sys_type direction
%type <tydef> type_definition
%type <functiondef> function_head procedure_head function_decl procedure_decl parameters para_decl_list para_type_list
%type <expVal> expression_list args_list
%type <str> name_list var_para_list val_para_list
%type <tyVal> field_decl_list field_decl
%type <labelVal> label_part label_list
%type <constVal> const_part const_expr_list
%type <typeVal> type_part type_decl_list  
%type <varVal> var_part var_decl_list var_decl 
%type <funVal> routine_part
%type <situationVal> case_expr_list
%nonassoc "then"
%nonassoc T_ELSE
%nonassoc T_PROCEDURE
%nonassoc T_FUNCTION

%%
program:              program_head routine T_DOT        {ast_root = $1; ast_root->addRoutine($2);}
                    ;
    
program_head:         T_PROGRAM S_ID T_SEMI             {$$ = new Program(symtab[$2].id);}
                    ;

routine:              routine_head routine_body         {$$ = new Routine($1, $2);} // Define, Body
                    ;

sub_routine:          routine_head routine_body         {$$ = new Routine($1, $2);}
                    ;

routine_head:         label_part const_part type_part var_part routine_part     {$$ = new Define(*$1, *$2, *$3, *$4, *$5);} 
                    ;

label_part:           T_LABEL label_list T_SEMI         {$$ = $2;}
                    | /* empty */                       {$$ = new std::vector<LabelDef*>();} 
                    ;

label_list:           label_list T_COMMA S_INTEGER      {$$ = $1; $$->push_back(new LabelDef($3));}       
                    | S_INTEGER                         {$$ = new std::vector<LabelDef *>(); $$->push_back(new LabelDef($1));}
                    ;

const_part:           T_CONST const_expr_list           {$$ = $2;}  // std::vector<ConstDef *> *
                    | /* empty */                       {$$ = new std::vector<ConstDef *>();}
                    ;

const_expr_list:      const_expr_list S_ID T_EQUAL const_value T_SEMI           {$$ = $1; $$->push_back(new ConstDef(symtab[$2].id, (Exp*)$4));} // TODO
                    | S_ID T_EQUAL const_value T_SEMI   {$$ = new std::vector<ConstDef *>(); $$->push_back(new ConstDef(symtab[$1].id, (Exp*)$3));}
                    ;

const_value:          S_INTEGER                         {Value* value= new Value; value->base_type = TY_INTEGER; value->val.integer_value = $1; $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_INTEGER);} // ConstantExp可能有转化问题
                    | S_REAL                            {Value* value= new Value; value->base_type = TY_REAL; value->val.real_value = atof(symtab[$1].id); $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_REAL);}
                    | S_CHAR                            {Value* value= new Value; value->base_type = TY_CHAR; value->val.char_value = symtab[$1].id[0]; $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_CHAR);}
                    | S_STRING                          {Value* value= new Value; value->base_type = TY_STRING; value->val.string_value = new string(symtab[$1].id); $$ = new ConstantExp(value); ((Exp*)$$)->return_type = new Type(TY_STRING);}
                    | sys_con                           {$$ = $1;}
                    ;

sys_con:              T_TRUE                            {Value* value= new Value; value->base_type = TY_BOOLEAN; value->val.boolean_value = true; $$ = new ConstantExp(value); ((ConstantExp*)$$)->return_type = new Type(TY_BOOLEAN);}
                    | T_FALSE                           {Value* value= new Value; value->base_type = TY_BOOLEAN; value->val.boolean_value = false; $$ = new ConstantExp(value); ((ConstantExp*)$$)->return_type = new Type(TY_BOOLEAN);}
                    | T_MAXINT                          {Value* value= new Value; value->base_type = TY_INTEGER; value->val.integer_value = 32767; $$ = new ConstantExp(value); ((ConstantExp*)$$)->return_type = new Type(TY_INTEGER);}
                    ;

type_part:            T_TYPE type_decl_list             {$$ = $2; tmp = *$$;}
                    | /* empty */                       {$$ = new std::vector<TypeDef *>();}
                    ;

type_decl_list:       type_decl_list type_definition    {$$ = $1; $$->push_back($2);}
                    | type_definition                   {$$ = new std::vector<TypeDef *> (); $$->push_back($1);}
                    ;

type_definition:      S_ID T_EQUAL type_decl T_SEMI     {$$ = new TypeDef(symtab[$1].id, $3);} // $3 TypeDef
                    ;

type_decl:            simple_type_decl                  {$$ = $1;} // Type
                    | array_type_decl                   {$$ = $1;}
                    | record_type_decl                  {$$ = $1;}
                    ;

sys_type:             T_CHAR                            {$$ = new Type(TY_CHAR);}
                    | T_INTEGER                         {$$ = new Type(TY_INTEGER);}
                    | T_REAL                            {$$ = new Type(TY_REAL);}
                    | T_BOOLEAN                         {$$ = new Type(TY_BOOLEAN);}
                    | T_STRING                          {$$ = new Type(TY_STRING);}
                    ;

simple_type_decl:     sys_type                          {$$ = $1;} // Type
                    | S_ID                              {bool flag = false; for(auto tdef : tmp) {if(tdef->name == symtab[$1].id) {$$ = tdef->type; flag = true;}} if(flag==false) yyerror("Semantics Error: Undefined type");} // we define
                    // | T_LP name_list T_RP               {$$ = $2;}
                    | const_value T_DOTDOT const_value  {$$ = new Type(TY_ARRAY); $$->array_start = ((ConstantExp*)$1)->value->val.integer_value; 
                                                                                  $$->array_end = ((ConstantExp*)$3)->value->val.integer_value;
                                                        }
                    ;

array_type_decl:      T_ARRAY T_LB simple_type_decl T_RB T_OF type_decl         {$$ = $3; $$->child_type.push_back($6);}
                    ;

record_type_decl:     T_RECORD field_decl_list T_END    {$$ = new Type(TY_RECORD); $$->child_type = *$2;} // TODO TY_ROCORD type
                    ;

field_decl_list:      field_decl_list field_decl        {$$ = $1; $$->insert($$->end(), $2->begin(), $2->end());} // std::vector<Type *>
                    | field_decl                        {$$ = new std::vector<Type *>(); $$->insert($$->end(), $1->begin(), $1->end());} 
                    ;

field_decl:           name_list T_COLON type_decl T_SEMI {$$ = new std::vector<Type *>(); for(auto iter : *$1) {Type * tmp = new Type(); tmp->name = iter; tmp->child_type.push_back($3); $$->push_back(tmp);}} // std::vector<Type *>
                ;   

name_list:            name_list T_COMMA S_ID            {$$ = $1; $$->push_back(symtab[$3].id);}
                    | S_ID                              {$$ = new std::vector<string>(); $$->push_back(symtab[$1].id);}
                    ; // TODO std::vector<string>

var_part:             T_VAR var_decl_list               {$$ = $2;}
                    |/* empty */                        {$$ = new std::vector<VarDef *>();}
                    ;

var_decl_list:        var_decl_list var_decl            {$$ = $1; $$->insert($$->end(), $2->begin(), $2->end());}
                    | var_decl                          {$$ = $1;} // std::vector<VarDef *>
                    ;

var_decl:             name_list T_COLON type_decl T_SEMI {$$ = new std::vector<VarDef*>(); for(auto iter : *$1) {$$->push_back(new VarDef(iter, $3));}} // TODO std::vector
                    ;

routine_part:         routine_part function_decl        {$$ = $1; $$->push_back($2);} // std::vector <FunctionDef *>
                    | routine_part procedure_decl       {$$ = $1; $$->push_back($2);}
                    | function_decl                     {$$ = new std::vector<FunctionDef *>; $$->push_back($1);}
                    | procedure_decl                    {$$ = new std::vector<FunctionDef *>; $$->push_back($1);}
                    | %prec "then"                      {$$ = new std::vector<FunctionDef *>();} /* empty */
                    ;

function_decl:        function_head T_SEMI sub_routine T_SEMI                   {$$ = $1; $$->addBody($3->body); $$->addDefine($3->define);} // FunctionDef *
                    ;

function_head:        T_FUNCTION S_ID parameters T_COLON simple_type_decl       {$$ = $3; $$->name = symtab[$2].id; $$->setReturnType($5); for(auto argt: $$->args_type) {argt->father = $$;}} // TODO                                                                 
                    ;

procedure_decl:       procedure_head T_SEMI sub_routine T_SEMI                  {$$ = $1; $$->addBody($3->body); $$->addDefine($3->define);}
                    ;

procedure_head:       T_PROCEDURE S_ID parameters       {$$ = $3; $$->name = symtab[$2].id;} // TODO
                    ;

parameters:           T_LP para_decl_list T_RP          {$$ = $2;}
                    | /* empty */                       {$$ = new FunctionDef("tmp");}
                    ;

para_decl_list:       para_decl_list T_SEMI para_type_list                      {$$ = $1; $$->args_name.insert($$->args_name.end(), $3->args_name.begin(), $3->args_name.end());
                                                                                        $$->args_type.insert($$->args_type.end(), $3->args_type.begin(), $3->args_type.end());
                                                                                        $$->args_is_formal_parameters.insert($$->args_is_formal_parameters.end(), $3->args_is_formal_parameters.begin(), $3->args_is_formal_parameters.end());}
                    | para_type_list                                            {$$ = $1;}
                    ;

para_type_list:       var_para_list T_COLON simple_type_decl                    {$$ = new FunctionDef("tmp"); for(auto iter : *$1) {$$->addArgs(iter, $3, true);}} // true args, type, bool
                    | val_para_list T_COLON simple_type_decl                    {$$ = new FunctionDef("tmp"); for(auto iter : *$1) {$$->addArgs(iter, $3, false);}} // false
                    ;

var_para_list:        T_VAR name_list                   {$$ = $2;} // std::vector <string> procedure true
                    ;

val_para_list:        name_list                         {$$ = $1;} // std::vector <string>
                    ;

routine_body:         compound_stmt                     {$$ = $1;} // Body
                    ;

compound_stmt:        T_BEGIN stmt_list T_END           {$$ = $2;} // Body
                    ;

stmt_list:            stmt_list stmt T_SEMI             {$$ = $1; for(auto iter : $2->stms) {$$->addStm(iter);}} // Body
                    | /* empty */                       {$$ = new Body();}
                    ;

stmt:                 S_INTEGER T_COLON non_label_stmt  {$$ = new Body();} // TODO
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

assign_stmt:          S_ID T_ASSIGN expression          {$$ = new AssignStm(new VariableExp(symtab[$1].id), $3);}
                    | S_ID T_LB expression T_RB T_ASSIGN expression    {$$ = new AssignStm(new BinaryExp(OP_INDEX, new VariableExp(symtab[$1].id), $3), $6);}
                    | S_ID T_DOT S_ID T_ASSIGN expression              {$$ = new AssignStm(new BinaryExp(OP_DOT, new VariableExp(symtab[$1].id), new VariableExp(symtab[$3].id)), $5);}
                    ;

proc_stmt:            S_ID                              {$$ = new CallStm(symtab[$1].id);}
                    | S_ID T_LP args_list T_RP          {$$ = new CallStm(symtab[$1].id); for(auto stm : *$3) ((CallStm*)$$)->addArgs(stm);} // TODO
                    | sys_proc T_LP expression_list T_RP {$$ = $1; for(auto stm : *$3) ((CallStm*)$$)->addArgs(stm);}
                    | sys_proc T_LP factor T_RP         {$$ = $1; ((CallStm*)$$)->addArgs($3);}
                    ;

sys_proc:             T_WRITE                           {$$ = new CallStm("write");}
                    | T_WRITELN                         {$$ = new CallStm("writeln");}
                    | T_READ                            {$$ = new CallStm("read");}
                    ;

if_stmt:              T_IF expression T_THEN stmt else_clause  {$$ = new IfStm(); ((IfStm*)$$)->setCondition($2); ((IfStm*)$$)->addTrue($4); ((IfStm*)$$)->addFalse($5);}
                    ; // IfStm : Exp Body Body

else_clause:          T_ELSE stmt                       {$$ = $2;} // Body
                    | %prec "then"                      {$$ = new Body();} /* empty */
                    ;

repeat_stmt:          T_REPEAT stmt_list T_UNTIL expression      {$$ = new RepeatStm(); ((RepeatStm*)$$)->setCondition($4);((RepeatStm*)$$)->addLoop($2);}
                    ;

while_stmt:           T_WHILE expression T_DO stmt      {$$ = new WhileStm($2); ((WhileStm*)$$)->addLoop($4);}
                    ;

for_stmt:             T_FOR S_ID T_ASSIGN expression direction expression T_DO stmt {$$ = new ForStm(symtab[$2].id, $4, $6, $5->base_type); ((ForStm*)$$)->addLoop($8);}
                    ; // iter Exp int Exp Body

direction:            T_TO                              {$$ = new Type(1);}
                    | T_DOWNTO                          {$$ = new Type(-1);}
                    ;

case_stmt:            T_CASE expression T_OF case_expr_list T_END               {$$ = new CaseStm($2); for(auto situ : *$4) ((CaseStm*)$$)->addSituation(situ);} // TODO
                    ;

case_expr_list:       case_expr_list case_expr          {$$ = $1; ((CaseStm*)$$)->addSituation((Situation*)$2);}
                    | case_expr                         {$$ = new std::vector<Situation *>(); ((CaseStm*)$$)->addSituation((Situation*)$1);}
                    ;

case_expr:            const_value T_COLON stmt T_SEMI   {$$ = new Situation(); ((Situation*)$$)->addMatch($1); ((Situation*)$$)->addSolution($3);} // Situation
                    | S_ID T_COLON stmt T_SEMI          {$$ = new Situation(); ((Situation*)$$)->addMatch(new VariableExp(symtab[$1].id)); ((Situation*)$$)->addSolution($3);} //TODO
                    ;

goto_stmt:            T_GOTO S_INTEGER                  {$$ = new GotoStm($2);}
                    ;

expression_list:      expression_list T_COMMA expression {$$ = $1; $$->push_back($3);} // std::vector<Exp *>
                    | expression                        {$$ = new std::vector<Exp *>(); $$->push_back($1);}
                    ;

expression:           expression T_GE expr              {$$ = new BinaryExp(OP_LARGE_EQUAL, $1, $3);}
                    | expression T_GT expr              {$$ = new BinaryExp(OP_LARGE, $1, $3);}
                    | expression T_LE expr              {$$ = new BinaryExp(OP_SMALL_EQUAL, $1, $3);}
                    | expression T_LT expr              {$$ = new BinaryExp(OP_SMALL, $1, $3);}
                    | expression T_EQUAL expr           {$$ = new BinaryExp(OP_EQUAL, $1, $3);}
                    | expression T_NE expr              {$$ = new BinaryExp(OP_NOT_EQUAL, $1, $3);}
                    | expr                              {$$ = $1;}
                    ;

expr:                 expr T_PLUS term                  {$$ = new BinaryExp(OP_ADD, $1, $3);}
                    | expr T_MINUS term                 {$$ = new BinaryExp(OP_MINUS, $1, $3);}
                    | expr T_OR term                    {$$ = new BinaryExp(OP_OR, $1, $3);}
                    | term                              {$$ = $1;}
                    ;

term:                 term T_MUL factor                 {$$ = new BinaryExp(OP_MULTI, $1, $3);}
                    | term T_DIV factor                 {$$ = new BinaryExp(OP_RDIV, $1, $3);}
                    | term T_MOD factor                 {$$ = new BinaryExp(OP_MOD, $1, $3);}
                    | term T_AND factor                 {$$ = new BinaryExp(OP_AND, $1, $3);}
                    | factor                            {$$ = $1;}
                    ;

sys_funct:            T_ABS                             {$$ = new CallExp("T_ABS");}
                    | T_CHR                             {$$ = new CallExp("T_CHR");}
                    | T_ODD                             {$$ = new CallExp("T_ODD");}
                    | T_ORD                             {$$ = new CallExp("T_ORD");}
                    | T_PRED                            {$$ = new CallExp("T_PRED");}
                    | T_SQR                             {$$ = new CallExp("T_SQR");}
                    | T_SQRT                            {$$ = new CallExp("T_SQRT");}
                    | T_SUCC                            {$$ = new CallExp("T_SUCC");}
                    ;


factor:               S_ID                              {$$ = new VariableExp(symtab[$1].id);}
                    | S_ID T_LP args_list T_RP          {$$ = new CallExp(symtab[$1].id); for(auto stm : *$3) ((CallExp*)$$)->addArgs(stm);} // args_list is a std::vector<Exp*>
                    | sys_funct                         {$$ = $1;}
                    | sys_funct T_LP args_list T_RP     {$$ = $1; for(auto stm : *$3) ((CallExp*)$$)->addArgs(stm);} // args_list is a std::vector<Exp*>
                    | const_value                       {$$ = $1;}
                    | T_LP expression T_RP              {$$ = $2;}
                    | T_NOT factor                      {$$ = new UnaryExp(OP_NOT, $2);}
                    | T_MINUS factor                    {$$ = new UnaryExp(OP_MINUS, $2);}
                    | S_ID T_LB expression T_RB         {$$ = new BinaryExp(OP_INDEX, new VariableExp(symtab[$1].id), $3);}
                    | S_ID T_DOT S_ID                   {$$ = new BinaryExp(OP_DOT, new VariableExp(symtab[$1].id), new VariableExp(symtab[$3].id));}
                    ;

args_list:            args_list T_COMMA expression      {$$ = $1; $$->push_back($3);} // std::vector<Exp*>
                    | expression                        {$$ = new std::vector<Exp *>; $$->push_back($1);}
                    ;

%%

int doyyparse(char *file) {
    FILE *fp;
    if ((fp = fopen(file, "r")) != NULL) {
        yyin = fp;
    }
    do {
        yyparse();
    } while (!feof(yyin));

    return 0;
}

