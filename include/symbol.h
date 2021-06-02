#ifndef SYMBOL_H
#define SYMBOL_H
#include <string>

#define ID_LENTH 50
#define symbolTable_SIZE 50

enum Op_Type {
    OP_ADD,
    OP_MINUS,
    OP_MULTI,
    OP_RDIV,
    OP_DDIV,
    OP_MOD,
    OP_AND,
    OP_OR,
    OP_SMALL,
    OP_LARGE,
    OP_SMALL_EQUAL,
    OP_LARGE_EQUAL,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_DOT,
    OP_INDEX,
    OP_OPPO,
    OP_ABS,
    OP_PRED,
    OP_SUCC,
    OP_ODD,
    OP_CHR,
    OP_ORD,
    OP_NOT,
    OP_SQR,
    OP_SQRT
};

enum Node_Type {
    ND_PROGRAM,
    ND_DEFINE,
    ND_BODY,
    ND_SITUATION,

    ND_LABEL_DEF,
    ND_CONST_DEF,
    ND_TYPE_DEF,
    ND_VAR_DEF,
    ND_FUNC_DEF,
    ND_ARG_DEF,

    ND_ASSIGN_STM,
    ND_CALL_STM,
    ND_CASE_STM,
    ND_FOR_STM,
    ND_GOTO_STM,
    ND_IF_STM,
    ND_LABEL_STM,
    ND_REPEAT_STM,
    ND_WHILE_STM,

    ND_BINARY_EXP,
    ND_CALL_EXP,
    NX_CONST_EXP,
    ND_UNARY_EXP,
    ND_VARIABLE_EXP,
    ND_TYPE
};

enum Val_Type { TY_INTEGER, TY_REAL, TY_CHAR, TY_BOOLEAN, TY_STRING, TY_ARRAY, TY_RECORD };

std::string getOpNameByID(int id);
extern "C" void yyerror(const char *info);

struct symbolTableTreeNode {
    char id[ID_LENTH];
    char type;
};

#endif
