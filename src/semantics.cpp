#include "symbol.h"
#include "tree.h"

using namespace tree;

void yyerror(Base *err_node, const char *info) {
    fprintf(stderr, "%p: %s\n", err_node, info);
}

bool isTypeBoolean(Type *type) {
    return type->baseType == TY_BOOLEAN;
}

bool isTypeInt(Type *type) {
    return type->baseType == TY_INTEGER;
}

bool isTypeReal(Type *type) {
    return type->baseType == TY_REAL;
}

bool isTypeChar(Type *type) {
    return type->baseType == TY_CHAR;
}

bool isTypeRecord(Type *type) {
    return type->baseType == TY_RECORD;
}

bool isTypeArray(Type *type) {
    return type->baseType == TY_ARRAY;
}

bool isTypeString(Type *type) {
    return type->baseType == TY_STRING;
}

bool canFillTypeWithValue(Type *type, Value *value) {
    switch (type->baseType) {
        case TY_REAL: return value->baseType == TY_INTEGER || value->baseType == TY_REAL;
        case TY_INTEGER:
        case TY_CHAR:
        case TY_BOOLEAN: return value->baseType == type->baseType;
        case TY_STRING: return value->baseType == TY_CHAR || value->baseType == TY_STRING;
        case TY_ARRAY:
            if (value->baseType != TY_ARRAY)
                return false;
            if (type->indexEnd - type->indexStart + 1 != (*(value->val.childValVec)).size())
                return false;
            for (int i = 0; i <= type->indexEnd - type->indexStart; i++)
                if (!canFillTypeWithValue(type->childType[i], (*(value->val.childValVec))[i]))
                    return false;
            return true;
        case TY_RECORD:
            if (value->baseType != TY_RECORD)
                return false;
            if (type->childType.size() != (*(value->val.childValVec)).size())
                return false;
            for (int i = 0; i < type->childType.size(); i++)
                if (!canFillTypeWithValue(type->childType[0], (*(value->val.childValVec))[i]))
                    return false;
            return true;
        default: return false;
    }
}

Type *generateTypeByValue(Value *value) {
    Type *type;
    switch (value->baseType) {
        case TY_INTEGER:
        case TY_REAL:
        case TY_CHAR:
        case TY_BOOLEAN:
        case TY_STRING: type = new Type(value->baseType); break;
        case TY_RECORD:
            type = new Type(value->baseType);
            for (auto val : (*(value->val.childValVec)))
                type->childType.push_back(generateTypeByValue(val));
            break;
        case TY_ARRAY:
            type             = new Type(TY_ARRAY);
            type->indexStart = 0;
            type->indexEnd   = (*(value->val.childValVec)).size() - 1;
            type->childType.push_back(generateTypeByValue((*(value->val.childValVec))[0]));
            break;
        default: type = nullptr;
    }
    return type;
}

Type *findChildType(Type *pType, const std::string &basic_string) {
    if (pType->baseType != TY_RECORD)
        return nullptr;
    for (Type *child : pType->childType)
        if (child->name == basic_string)
            return child;
    return nullptr;
}

bool canFindChild(Type *pType, const std::string &basic_string) {
    if (pType->baseType != TY_RECORD)
        return false;
    for (Type *child : pType->childType)
        if (child->name == basic_string)
            return true;
    return false;
}

// block object
bool Program::checkSemantics() {
    // check children
    isLegal &= define->checkSemantics();
    if (isLegal)
        isLegal &= body->checkSemantics();
    // check between children
    // check itself
    return isLegal;
}

bool Define::checkSemantics() {
    // check children
    for (LabelDef *iter : labelDef)
        isLegal &= iter->checkSemantics();
    for (ConstDef *iter : constDef)
        isLegal &= iter->checkSemantics();
    for (TypeDef *iter : typeDef)
        isLegal &= iter->checkSemantics();
    for (VarDef *iter : varDef)
        isLegal &= iter->checkSemantics();
    for (FuncDef *iter : funcDef)
        isLegal &= iter->checkSemantics();
    // check between children
    if (isLegal)
        for (ConstDef *c_iter : constDef) {
            if (isLegal)
                for (TypeDef *t_iter : typeDef)
                    if (c_iter->name == t_iter->name) {
                        isLegal = false;
                        break;
                    } else
                        ;
            else
                break;
            if (isLegal)
                for (VarDef *v_iter : varDef)
                    if (c_iter->name == v_iter->name) {
                        isLegal = false;
                        break;
                    } else
                        ;
            else
                break;
            if (isLegal)
                for (FuncDef *f_iter : funcDef)
                    if (c_iter->name == f_iter->name) {
                        isLegal = false;
                        break;
                    } else
                        ;
            else
                break;
        }
    if (isLegal)
        for (TypeDef *t_iter : typeDef) {
            if (isLegal)
                for (VarDef *v_iter : varDef)
                    if (t_iter->name == v_iter->name) {
                        isLegal = false;
                        break;
                    } else
                        ;
            else
                break;
            if (isLegal)
                for (FuncDef *f_iter : funcDef)
                    if (t_iter->name == f_iter->name) {
                        isLegal = false;
                        break;
                    } else
                        ;
            else
                break;
        }
    if (isLegal)
        for (VarDef *v_iter : varDef) {
            if (isLegal)
                for (FuncDef *f_iter : funcDef)
                    if (v_iter->name == f_iter->name) {
                        isLegal = false;
                        break;
                    } else
                        ;
            else
                break;
        }
    if (!isLegal) {
        char info[200];
        sprintf(info, "Semantics Error: There are at least two obeject in define part, which "
                      "has the same name.");
        yyerror(this, info);
    }
    // check itself
    return isLegal;
}

bool Body::checkSemantics() {
    // check children
    for (Stm *iter : stms)
        isLegal &= iter->checkSemantics();
    // check between children
    // check itself
    return isLegal;
}

bool Situation::checkSemantics() {
    // check children
    for (Exp *iter : caseVec)
        isLegal &= iter->checkSemantics();
    isLegal &= solution->checkSemantics();
    // check between children
    // check itself
    return isLegal;
}

// define object
bool LabelDef::checkSemantics() {
    // check children
    // check between children
    // check itself
    isLegal = true;
    return isLegal;
}

bool ConstDef::checkSemantics() {
    // check children
    // check between children
    // check itself
    isLegal = true;
    return isLegal;
}

bool TypeDef::checkSemantics() {
    // check children
    // check between children
    // check itself
    isLegal = true;
    return isLegal;
}

bool VarDef::checkSemantics() {
    // check children
    // check between children
    // check itself
    isLegal = true;
    return isLegal;
}

bool FuncDef::checkSemantics() {
    // check children
    isLegal &= define->checkSemantics();
    if (isLegal) {
        for (std::string arg_name : argNameVec) {
            if (arg_name == name) {
                isLegal = false;
                break;
            }
            if (isLegal)
                for (ConstDef *iter : define->constDef)
                    if (iter->name == arg_name) {
                        isLegal = false;
                        break;
                    }
            if (isLegal)
                for (TypeDef *iter : define->typeDef)
                    if (iter->name == arg_name) {
                        isLegal = false;
                        break;
                    }
            if (isLegal)
                for (VarDef *iter : define->varDef)
                    if (iter->name == arg_name) {
                        isLegal = false;
                        break;
                    }
            if (isLegal)
                for (FuncDef *iter : define->funcDef)
                    if (iter->name == arg_name) {
                        isLegal = false;
                        break;
                    }
        }
    }
    if (isLegal)
        for (ConstDef *iter : define->constDef)
            if (iter->name == name) {
                isLegal = false;
                break;
            }
    if (isLegal)
        for (TypeDef *iter : define->typeDef)
            if (iter->name == name) {
                isLegal = false;
                break;
            }
    if (isLegal)
        for (VarDef *iter : define->varDef)
            if (iter->name == name) {
                isLegal = false;
                break;
            }
    if (isLegal)
        for (FuncDef *iter : define->funcDef)
            if (iter->name == name) {
                isLegal = false;
                break;
            }
    if (isLegal)
        isLegal = body->checkSemantics();
    else {
        char info[200];
        sprintf(info,
            "Semantics Error: There are at least two obeject in function %s, which has the "
            "same name.",
            name.c_str());
        yyerror(this, info);
    }
    // check between children
    // check itself
    return isLegal;
}

// stm
bool AssignStm::checkSemantics() {
    // check children
    isLegal = leftVal->checkSemantics() && rightVal->checkSemantics();
    // check between children
    if (isLegal)
        isLegal = isSameType(leftVal->returnType, rightVal->returnType);
    // check itself
    if (isLegal)
        if (leftVal->returnVal != nullptr) {
            isLegal = false;
            char info[200];
            sprintf(info, "Semantics Error: Constant obeject cannot be the left value in an "
                          "assignment statement.");
            yyerror(this, info);
        } else
            ;
    else {
        char info[200];
        sprintf(info, "Semantics Error: Left value and right value must be the same type in "
                      "an assignment statement.");
        yyerror(this, info);
    }
    return isLegal;
}

bool CallStm::checkSemantics() {
    // check children
    for (Exp *iter : args)
        isLegal &= iter->checkSemantics();
    // check between children
    // check itself
    if (name == "write" || name == "writeln") {
        isLegal = true;
    } else {
        FuncDef *function = findFunction(name, this->father);
        if (function == nullptr) {
            isLegal = false;
            char info[200];
            sprintf(info, "Semantics Error: Cannot find function %s to call.", name.c_str());
            yyerror(this, info);
        } else if (args.size() != function->argTypeVec.size()) {
            isLegal = false;
            char info[200];
            sprintf(info,
                "Semantics Error: The number of arguments are different between call and "
                "definition of function %s.",
                name.c_str());
            yyerror(this, info);
        } else {
            for (int i = 0; i < args.size(); i++)
                if (!isSameType(args[i]->returnType, function->argTypeVec[i])) {
                    isLegal = false;
                    char info[200];
                    sprintf(info,
                        "Semantics Error: No.%d argument has the differenct type between "
                        "call and definition of function %s.",
                        i, name.c_str());
                    yyerror(this, info);
                    break;
                }
        }
    }
    return isLegal;
}

bool CaseStm::checkSemantics() {
    // 全常量检测
    for (auto situation : situations)
        for (auto match_item : situation->caseVec)
            if (match_item->returnVal == nullptr) {
                char info[200];
                sprintf(info,
                    "Semantics Error: The match items in case statement must be constant.");
                yyerror(this, info);
                isLegal = false;
                return isLegal;
            }
    // 类型匹配
    bool is_int = isTypeInt(object->returnType);
    for (auto situation : situations)
        for (auto match_item : situation->caseVec)
            if (is_int != isTypeInt(match_item->returnType)) {
                char info[200];
                sprintf(info, "Semantics Error: The match items in case statement must have "
                              "the same type as the switch object.");
                yyerror(this, info);
                isLegal = false;
                return isLegal;
            }
    // 重复值检测
    bool flag[65536];
    for (bool &i : flag)
        i = false;
    for (auto situation : situations)
        for (Exp *match_item : situation->caseVec) {
            int id = is_int ? (match_item->returnVal->val.intVal + 32768)
                            : ((int)match_item->returnVal->val.charVal);
            if (flag[id]) {
                char info[200];
                sprintf(info,
                    "Semantics Error: The match items in case statement must be different.");
                yyerror(this, info);
                isLegal = false;
                return isLegal;
            }
            flag[id] = true;
        }
    return isLegal;
}

bool ForStm::checkSemantics() {
    start->checkSemantics();
    end->checkSemantics();
    if (!((isTypeInt(start->returnType) && isTypeInt(end->returnType)) ||
            (isTypeChar(start->returnType) && isTypeChar(end->returnType)))) {
        char info[200];
        sprintf(info, "Semantics Error: The start index or the end index is illegal.");
        yyerror(this, info);
        isLegal = false;
    }
    isLegal &= loop->checkSemantics();
    return isLegal;
}

bool GotoStm::checkSemantics() {
    // check children
    // check between children
    // check itself
    isLegal &= canFindLabel(label, this->father);
    if (!isLegal) {
        char info[200];
        sprintf(info, "Semantics Error: Cannot find label %d.", label);
        yyerror(this, info);
    }
    return isLegal;
}

bool IfStm::checkSemantics() {
    // check children
    isLegal &= condition->checkSemantics();
    isLegal &= trueBody->checkSemantics();
    if (falseBody != nullptr)
        isLegal &= falseBody->checkSemantics();
    // check between children
    // check itself
    if (isTypeBoolean(condition->returnType))
        isLegal &= true;
    else {
        char info[200];
        sprintf(info, "Semantics Error: The type for the condition must be boolean.");
        yyerror(this, info);
        isLegal = false;
    }
    return isLegal;
}

bool LabelStm::checkSemantics() {
    // check children
    // check between children
    // check itself
    isLegal &= canFindLabel(label, this->father);
    if (!isLegal) {
        char info[200];
        sprintf(info, "Semantics Error: Cannot find label %d.", label);
        yyerror(this, info);
    }
    return isLegal;
}

bool RepeatStm::checkSemantics() {
    // check children
    isLegal &= condition->checkSemantics();
    isLegal &= loop->checkSemantics();
    // check between children
    // check itself
    if (isTypeBoolean(condition->returnType))
        isLegal &= true;
    else {
        char info[200];
        sprintf(info, "Semantics Error: The type for the condition must be boolean.");
        yyerror(this, info);
        isLegal = false;
    }
    return isLegal;
}

bool WhileStm::checkSemantics() {
    // check children
    isLegal &= condition->checkSemantics();
    isLegal &= loop->checkSemantics();
    // check between children
    // check itself
    if (isTypeBoolean(condition->returnType))
        isLegal &= true;
    else {
        char info[200];
        sprintf(info, "Semantics Error: The type for the condition must be boolean.");
        yyerror(this, info);
        isLegal = false;
    }
    return isLegal;
}

// exp
bool UnaryExp::checkSemantics() {
    isLegal = operand->checkSemantics();
    if (isLegal)
        switch (opcode) {
            case OP_OPPO: {
                if (!isTypeInt(operand->returnType) && !isTypeReal(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of an operand with an unary "
                                  "operator \'-\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = copyType(operand->returnType);
                }
            } break;
            case OP_NOT: {
                if (!isTypeChar(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of an operand with an unary "
                                  "operator \'not\' must be boolean.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_ABS: {
                if (!isTypeInt(operand->returnType) && !isTypeReal(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the parameter in function "
                                  "\'abs\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = copyType(operand->returnType);
                }
            } break;
            case OP_PRED: {
                if (!isTypeChar(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the parameter in function "
                                  "\'pred\' must be char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_CHAR);
                }
            } break;
            case OP_SUCC: {
                if (!isTypeChar(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the parameter in function "
                                  "\'succ\' must be char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_CHAR);
                }
            } break;
            case OP_ODD: {
                if (!isTypeInt(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the parameter in function "
                                  "\'odd\' must be integer.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_CHR: {
                if (!isTypeInt(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the parameter in function "
                                  "\'chr\' must be integer.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_CHAR);
                }
            } break;
            case OP_ORD: {
                if (!isTypeChar(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the parameter in function "
                                  "\'ord\' must be char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_INTEGER);
                }
            } break;
            case OP_SQR: {
                if (!isTypeInt(operand->returnType) && !isTypeReal(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the parameter in function "
                                  "\'sqr\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_REAL);
                }
            } break;
            case OP_SQRT: {
                if (!isTypeInt(operand->returnType) && !isTypeReal(operand->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the parameter in function "
                                  "\'sqrt\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_REAL);
                }
            } break;
            default: {
                char info[200];
                sprintf(info, "Semantics Error: There is something wrong. This operator type "
                              "is unrecognised.");
                yyerror(this, info);
                isLegal = false;
            }
        }
    return isLegal;
}

bool BinaryExp::checkSemantics() {
    isLegal = operand1->checkSemantics();
    if (isLegal)
        switch (opcode) {
            case OP_ADD: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'+\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    if (isTypeReal(operand1->returnType) || isTypeReal(operand2->returnType))
                        returnType = new Type(TY_REAL);
                    else
                        returnType = new Type(TY_INTEGER);
                }
            } break;
            case OP_MINUS: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'-\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    if (isTypeReal(operand1->returnType) || isTypeReal(operand2->returnType))
                        returnType = new Type(TY_REAL);
                    else
                        returnType = new Type(TY_INTEGER);
                }
            } break;
            case OP_MULTI: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'*\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    if (isTypeReal(operand1->returnType) || isTypeReal(operand2->returnType))
                        returnType = new Type(TY_REAL);
                    else
                        returnType = new Type(TY_INTEGER);
                }
            } break;
            case OP_RDIV: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'/\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_REAL);
                }
            } break;
            case OP_DDIV: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'div\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_INTEGER);
                }
            } break;
            case OP_MOD: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if (!isTypeInt(operand1->returnType) || !isTypeInt(operand2->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'%%\' must be integer.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_INTEGER);
                }
            } break;
            case OP_AND: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if (!isTypeChar(operand1->returnType) || !isTypeChar(operand2->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'and\' must be boolean.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_OR: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if (!isTypeChar(operand1->returnType) || !isTypeChar(operand2->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'or\' must be boolean.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_SMALL: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType) &&
                        !isTypeChar(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType) &&
                        !isTypeChar(operand1->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'<\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_LARGE: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType) &&
                        !isTypeChar(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType) &&
                        !isTypeChar(operand1->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'>\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_SMALL_EQUAL: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType) &&
                        !isTypeChar(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType) &&
                        !isTypeChar(operand1->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'<=\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_LARGE_EQUAL: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType) &&
                        !isTypeChar(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType) &&
                        !isTypeChar(operand1->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'>=\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_EQUAL: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType) &&
                        !isTypeChar(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType) &&
                        !isTypeChar(operand1->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'=\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_NOT_EQUAL: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(operand1->returnType) && !isTypeReal(operand1->returnType) &&
                        !isTypeChar(operand1->returnType)) ||
                    (!isTypeInt(operand2->returnType) && !isTypeReal(operand2->returnType) &&
                        !isTypeChar(operand1->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'<>\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOLEAN);
                }
            } break;
            case OP_DOT: {
                if (isTypeRecord(operand1->returnType))
                    if (canFindChild(operand1->returnType, ((VariableExp *)operand2)->name))
                        returnType = copyType(findChildType(
                            operand1->returnType, ((VariableExp *)operand2)->name));
                    else {
                        char info[200];
                        sprintf(info,
                            "Semantics Error: Cannot find child named %s in this record.",
                            (((VariableExp *)operand2)->name).c_str());
                        yyerror(this, info);
                        isLegal = false;
                    }
                else {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the first operand in the "
                                  "binary operator \'.\' must be record.");
                    yyerror(this, info);
                    isLegal = false;
                }
            } break;
            case OP_INDEX: {
                isLegal &= operand2->checkSemantics();
                if (!isLegal)
                    return isLegal;
                if (isTypeArray(operand1->returnType) || isTypeString(operand1->returnType)) {
                    if (isTypeInt(operand2->returnType) || isTypeChar(operand2->returnType)) {
                        returnType = copyType(operand1->returnType->childType[0]);
                    } else {
                        char info[200];
                        sprintf(info, "Semantics Error: The index must be integer or char.");
                        yyerror(this, info);
                        isLegal = false;
                    }
                } else {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of the first operand in the "
                                  "binary operator \'[]\' must be array or string.");
                    yyerror(this, info);
                    isLegal = false;
                }
            } break;
            default: {
                char info[200];
                sprintf(info, "Semantics Error: There is something wrong. This operator type "
                              "is unrecognised.");
                yyerror(this, info);
                isLegal = false;
            }
        }
    return isLegal;
}

bool CallExp::checkSemantics() {
    // check children
    for (Exp *iter : args)
        isLegal &= iter->checkSemantics();
    // check between children
    // check itself
    FuncDef *function = findFunction(name, this->father);
    if (function == nullptr) {
        isLegal = false;
        char info[200];
        sprintf(info, "Semantics Error: Cannot find function %s to call.", name.c_str());
        yyerror(this, info);
    } else if (args.size() != function->argTypeVec.size()) {
        isLegal = false;
        char info[200];
        sprintf(info,
            "Semantics Error: The number of arguments are different between call and "
            "definition of function %s.",
            name.c_str());
        yyerror(this, info);
    } else {
        for (int i = 0; i < args.size(); i++)
            if (!isSameType(args[i]->returnType, function->argTypeVec[i])) {
                isLegal = false;
                char info[200];
                sprintf(info,
                    "Semantics Error: No.%d argument has different type between call and "
                    "definition of function %s.",
                    i, name.c_str());
                yyerror(this, info);
                break;
            }
        if (isLegal)
            returnType = copyType(function->retType);
    }
    return isLegal;
}

bool ConstantExp::checkSemantics() {
    returnType = generateTypeByValue(value);
    isLegal    = true;
    return isLegal;
}

bool VariableExp::checkSemantics() {
    // printf("breakpoint1\n");
    Base *temp = findName(name, this->father);
    if (temp == nullptr)
        isLegal = false;
    else
        // printf("breakpoint2\n");
        switch (temp->nodeType) {
            case ND_CONST_DEF: {
                auto *node = (ConstDef *)temp;
                isLegal    = true;
                returnType = node->value->returnType;
                returnVal  = node->value->returnVal;
            } break;
            case ND_VAR_DEF: {
                auto node  = (VarDef *)temp;
                isLegal    = true;
                returnType = node->type;
            } break;
            case ND_ARG_DEF: {
                auto *node = (ArgDef *)temp;
                isLegal    = true;
                returnType = node->type;
            } break;
            case ND_FUNC_DEF: {
                auto *node = (FuncDef *)temp;
                if (node->retType != nullptr) {
                    isLegal    = true;
                    returnType = node->retType;
                } else
                    isLegal = false;
            } break;
            default: isLegal = false;
        }
    return isLegal;
}
