#include "symbol.h"
#include "tree.h"

using namespace tree;

void yyerror(Base *errNode, const char *info) {
    fprintf(stderr, "%p: %s\n", errNode, info);
}

bool isTypeBoolean(Type *type) {
    return type->baseType == TY_BOOL;
}

bool isTypeInt(Type *type) {
    return type->baseType == TY_INT;
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
        case TY_REAL: return value->baseType == TY_INT || value->baseType == TY_REAL;
        case TY_INT:
        case TY_CHAR:
        case TY_BOOL: return value->baseType == type->baseType;
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
        case TY_INT:
        case TY_REAL:
        case TY_CHAR:
        case TY_BOOL:
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
    for (auto child : pType->childType)
        if (child->name == basic_string)
            return child;
    return nullptr;
}

bool canFindChild(Type *pType, const std::string &basic_string) {
    if (pType->baseType != TY_RECORD)
        return false;
    for (auto child : pType->childType)
        if (child->name == basic_string)
            return true;
    return false;
}

// block object
bool Program::SEMANT_CHECK_LEGAL() {
    isLegal &= define->SEMANT_CHECK_LEGAL();
    if (isLegal)
        isLegal &= body->SEMANT_CHECK_LEGAL();
    return isLegal;
}

bool Define::SEMANT_CHECK_LEGAL() {
    for (auto itor : labelDef)
        isLegal &= itor->SEMANT_CHECK_LEGAL();
    for (auto itor : constDef)
        isLegal &= itor->SEMANT_CHECK_LEGAL();
    for (auto itor : typeDef)
        isLegal &= itor->SEMANT_CHECK_LEGAL();
    for (auto itor : varDef)
        isLegal &= itor->SEMANT_CHECK_LEGAL();
    for (auto itor : funcDef)
        isLegal &= itor->SEMANT_CHECK_LEGAL();
    if (isLegal)
        for (auto cItor : constDef) {
            if (isLegal)
                for (auto tItor : typeDef) {
                    if (cItor->name == tItor->name) {
                        isLegal = false;
                        break;
                    }
                }
            else
                break;
            if (isLegal)
                for (auto vItor : varDef) {
                    if (cItor->name == vItor->name) {
                        isLegal = false;
                        break;
                    }
                }
            else
                break;
            if (isLegal)
                for (auto fItor : funcDef) {
                    if (cItor->name == fItor->name) {
                        isLegal = false;
                        break;
                    }
                }
            else
                break;
        }
    if (isLegal)
        for (auto tItor : typeDef) {
            if (isLegal)
                for (auto vItor : varDef) {
                    if (tItor->name == vItor->name) {
                        isLegal = false;
                        break;
                    }
                }
            else
                break;
            if (isLegal)
                for (auto fItor : funcDef) {
                    if (tItor->name == fItor->name) {
                        isLegal = false;
                        break;
                    }
                }
            else
                break;
        }
    if (isLegal)
        for (auto vItor : varDef) {
            if (isLegal) {
                for (auto fItor : funcDef)
                    if (vItor->name == fItor->name) {
                        isLegal = false;
                        break;
                    }
            } else
                break;
        }
    if (!isLegal) {
        yyerror(this, "Semantics Error: There are at least two obeject in define part, which "
                      "has the same name.");
    }
    return isLegal;
}

bool Body::SEMANT_CHECK_LEGAL() {
    for (auto itor : stms)
        isLegal &= itor->SEMANT_CHECK_LEGAL();
    return isLegal;
}

bool Situation::SEMANT_CHECK_LEGAL() {
    for (auto itor : caseVec)
        isLegal &= itor->SEMANT_CHECK_LEGAL();
    isLegal &= solution->SEMANT_CHECK_LEGAL();
    return isLegal;
}

// define object
bool LabelDef::SEMANT_CHECK_LEGAL() {
    isLegal = true;
    return isLegal;
}

bool ConstDef::SEMANT_CHECK_LEGAL() {
    isLegal = true;
    return isLegal;
}

bool TypeDef::SEMANT_CHECK_LEGAL() {
    isLegal = true;
    return isLegal;
}

bool VarDef::SEMANT_CHECK_LEGAL() {
    isLegal = true;
    return isLegal;
}

bool FuncDef::SEMANT_CHECK_LEGAL() {
    isLegal &= define->SEMANT_CHECK_LEGAL();
    if (isLegal) {
        for (std::string arg_name : VectorNamePara) {
            if (arg_name == name) {
                isLegal = false;
                break;
            }
            if (isLegal)
                for (auto itor : define->constDef)
                    if (itor->name == arg_name) {
                        isLegal = false;
                        break;
                    }
            if (isLegal)
                for (auto itor : define->typeDef)
                    if (itor->name == arg_name) {
                        isLegal = false;
                        break;
                    }
            if (isLegal)
                for (auto itor : define->varDef)
                    if (itor->name == arg_name) {
                        isLegal = false;
                        break;
                    }
            if (isLegal)
                for (auto itor : define->funcDef)
                    if (itor->name == arg_name) {
                        isLegal = false;
                        break;
                    }
        }
    }
    if (isLegal)
        for (auto itor : define->constDef)
            if (itor->name == name) {
                isLegal = false;
                break;
            }
    if (isLegal)
        for (auto itor : define->typeDef)
            if (itor->name == name) {
                isLegal = false;
                break;
            }
    if (isLegal)
        for (auto itor : define->varDef)
            if (itor->name == name) {
                isLegal = false;
                break;
            }
    if (isLegal)
        for (auto itor : define->funcDef)
            if (itor->name == name) {
                isLegal = false;
                break;
            }
    if (isLegal)
        isLegal = body->SEMANT_CHECK_LEGAL();
    else {
        char info[200];
        sprintf(info,
            "Semantics Error: There are at least two obeject in function %s, which has the "
            "same name.",
            name.c_str());
        yyerror(this, info);
    }
    return isLegal;
}

// stm
bool StatementAssign::SEMANT_CHECK_LEGAL() {
    isLegal = leftVal->SEMANT_CHECK_LEGAL() && rightVal->SEMANT_CHECK_LEGAL();
    if (isLegal)
        isLegal = isSameType(leftVal->returnType, rightVal->returnType);
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

bool CallStm::SEMANT_CHECK_LEGAL() {
    for (auto itor : args)
        isLegal &= itor->SEMANT_CHECK_LEGAL();
    if (name == "write" || name == "writeln") {
        isLegal = true;
    } else {
        FuncDef *function = findFunction(name, this->father);
        if (function == nullptr) {
            isLegal = false;
            char info[200];
            sprintf(info, "Semantics Error: Cannot find function %s to call.", name.c_str());
            yyerror(this, info);
        } else if (args.size() != function->TypeofVectorPara.size()) {
            isLegal = false;
            char info[200];
            sprintf(info,
                "Semantics Error: The number of arguments are different between call and "
                "definition of function %s.",
                name.c_str());
            yyerror(this, info);
        } else {
            for (int i = 0; i < args.size(); i++)
                if (!isSameType(args[i]->returnType, function->TypeofVectorPara[i])) {
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

bool CaseStm::SEMANT_CHECK_LEGAL() {
    // 全常量检测
    for (auto situation : situations)
        for (auto match_item : situation->caseVec)
            if (match_item->returnVal == nullptr) {
                char info[200];
                sprintf(info, "Semantics Error: The match items in case statement must be constant.");
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
        for (auto match_item : situation->caseVec) {
            int id = is_int ? (match_item->returnVal->val.intVal + 32768) : ((int)match_item->returnVal->val.charVal);
            if (flag[id]) {
                char info[200];
                sprintf(info, "Semantics Error: The match items in case statement must be different.");
                yyerror(this, info);
                isLegal = false;
                return isLegal;
            }
            flag[id] = true;
        }
    return isLegal;
}

bool ForStm::SEMANT_CHECK_LEGAL() {
    start->SEMANT_CHECK_LEGAL();
    end->SEMANT_CHECK_LEGAL();
    if (!((isTypeInt(start->returnType) && isTypeInt(end->returnType)) ||
            (isTypeChar(start->returnType) && isTypeChar(end->returnType)))) {
        char info[200];
        sprintf(info, "Semantics Error: The start index or the end index is illegal.");
        yyerror(this, info);
        isLegal = false;
    }
    isLegal &= loop->SEMANT_CHECK_LEGAL();
    return isLegal;
}

bool GotoStm::SEMANT_CHECK_LEGAL() {
    isLegal &= canFindLabel(label, this->father);
    if (!isLegal) {
        char info[200];
        sprintf(info, "Semantics Error: Cannot find label %d.", label);
        yyerror(this, info);
    }
    return isLegal;
}

bool IfStm::SEMANT_CHECK_LEGAL() {
    isLegal &= condition->SEMANT_CHECK_LEGAL();
    isLegal &= trueBody->SEMANT_CHECK_LEGAL();
    if (falseBody != nullptr)
        isLegal &= falseBody->SEMANT_CHECK_LEGAL();
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

bool LabelStm::SEMANT_CHECK_LEGAL() {
    isLegal &= canFindLabel(label, this->father);
    if (!isLegal) {
        char info[200];
        sprintf(info, "Semantics Error: Cannot find label %d.", label);
        yyerror(this, info);
    }
    return isLegal;
}

bool StatementRepeat::SEMANT_CHECK_LEGAL() {
    isLegal &= condition->SEMANT_CHECK_LEGAL();
    isLegal &= loop->SEMANT_CHECK_LEGAL();
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

bool WhileStm::SEMANT_CHECK_LEGAL() {
    isLegal &= condition->SEMANT_CHECK_LEGAL();
    isLegal &= loop->SEMANT_CHECK_LEGAL();
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
bool UnaryExp::SEMANT_CHECK_LEGAL() {
    isLegal = operand->SEMANT_CHECK_LEGAL();
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
                    returnType = new Type(TY_BOOL);
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
                    returnType = new Type(TY_BOOL);
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
                    returnType = new Type(TY_INT);
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
            } break;
        }
    return isLegal;
}

bool BinaryExp::SEMANT_CHECK_LEGAL() {
    isLegal = OPRFIRST->SEMANT_CHECK_LEGAL();
    if (isLegal)
        switch (opcode) {
            case OP_ADD: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'+\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    if (isTypeReal(OPRFIRST->returnType) || isTypeReal(OPRSECOND->returnType))
                        returnType = new Type(TY_REAL);
                    else
                        returnType = new Type(TY_INT);
                }
            } break;
            case OP_MINUS: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'-\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    if (isTypeReal(OPRFIRST->returnType) || isTypeReal(OPRSECOND->returnType))
                        returnType = new Type(TY_REAL);
                    else
                        returnType = new Type(TY_INT);
                }
            } break;
            case OP_MULTI: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'*\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    if (isTypeReal(OPRFIRST->returnType) || isTypeReal(OPRSECOND->returnType))
                        returnType = new Type(TY_REAL);
                    else
                        returnType = new Type(TY_INT);
                }
            } break;
            case OP_RDIV: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType))) {
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
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'div\' must be integer or real.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_INT);
                }
            } break;
            case OP_MOD: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if (!isTypeInt(OPRFIRST->returnType) || !isTypeInt(OPRSECOND->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'%%\' must be integer.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_INT);
                }
            } break;
            case OP_AND: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if (!isTypeBoolean(OPRFIRST->returnType) || !isTypeBoolean(OPRSECOND->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'and\' must be boolean.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOL);
                }
            } break;
            case OP_OR: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if (!isTypeChar(OPRFIRST->returnType) || !isTypeChar(OPRSECOND->returnType)) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'or\' must be boolean.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOL);
                }
            } break;
            case OP_SMALL: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType) &&
                        !isTypeChar(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType) &&
                        !isTypeChar(OPRFIRST->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'<\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOL);
                }
            } break;
            case OP_LARGE: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType) &&
                        !isTypeChar(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType) &&
                        !isTypeChar(OPRFIRST->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'>\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOL);
                }
            } break;
            case OP_SMALL_EQUAL: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType) &&
                        !isTypeChar(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType) &&
                        !isTypeChar(OPRFIRST->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'<=\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOL);
                }
            } break;
            case OP_LARGE_EQUAL: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType) &&
                        !isTypeChar(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType) &&
                        !isTypeChar(OPRFIRST->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'>=\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOL);
                }
            } break;
            case OP_EQUAL: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType) &&
                        !isTypeChar(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType) &&
                        !isTypeChar(OPRFIRST->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'=\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOL);
                }
            } break;
            case OP_NOT_EQUAL: {
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if ((!isTypeInt(OPRFIRST->returnType) && !isTypeReal(OPRFIRST->returnType) &&
                        !isTypeChar(OPRFIRST->returnType)) ||
                    (!isTypeInt(OPRSECOND->returnType) && !isTypeReal(OPRSECOND->returnType) &&
                        !isTypeChar(OPRFIRST->returnType))) {
                    char info[200];
                    sprintf(info, "Semantics Error: The type of operands with a binary "
                                  "operator \'<>\' must be integer, real or char.");
                    yyerror(this, info);
                    isLegal = false;
                } else {
                    returnType = new Type(TY_BOOL);
                }
            } break;
            case OP_DOT: {
                if (isTypeRecord(OPRFIRST->returnType))
                    if (canFindChild(OPRFIRST->returnType, ((VariableExp *)OPRSECOND)->name))
                        returnType = copyType(findChildType(OPRFIRST->returnType, ((VariableExp *)OPRSECOND)->name));
                    else {
                        char info[200];
                        sprintf(info, "Semantics Error: Cannot find child named %s in this record.",
                            (((VariableExp *)OPRSECOND)->name).c_str());
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
                isLegal &= OPRSECOND->SEMANT_CHECK_LEGAL();
                if (!isLegal)
                    return isLegal;
                if (isTypeArray(OPRFIRST->returnType) || isTypeString(OPRFIRST->returnType)) {
                    if (isTypeInt(OPRSECOND->returnType) || isTypeChar(OPRSECOND->returnType)) {
                        returnType = copyType(OPRFIRST->returnType->childType[0]);
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

bool CallExp::SEMANT_CHECK_LEGAL() {
    for (auto itor : args)
        isLegal &= itor->SEMANT_CHECK_LEGAL();
    FuncDef *function = findFunction(name, this->father);
    if (function == nullptr) {
        isLegal = false;
        char info[200];
        sprintf(info, "Semantics Error: Cannot find function %s to call.", name.c_str());
        yyerror(this, info);
    } else if (args.size() != function->TypeofVectorPara.size()) {
        isLegal = false;
        char info[200];
        sprintf(info,
            "Semantics Error: The number of arguments are different between call and "
            "definition of function %s.",
            name.c_str());
        yyerror(this, info);
    } else {
        for (int i = 0; i < args.size(); i++)
            if (!isSameType(args[i]->returnType, function->TypeofVectorPara[i])) {
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

bool EXPRESSIONConst::SEMANT_CHECK_LEGAL() {
    returnType = generateTypeByValue(value);
    isLegal    = true;
    return isLegal;
}

bool VariableExp::SEMANT_CHECK_LEGAL() {
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
