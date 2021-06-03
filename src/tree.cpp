#include "tree.h"
#include <fstream>
#include <iostream>

// functions for Program
void tree::Program::setDefination(Define *define) {
    if (this->define == nullptr) {
        define->father = this;
        this->define   = define;
    }
}
// functions for Define
void tree::Define::addLabel(LabelDef *def) {
    def->father = this;
    labelDef.push_back(def);
}

void tree::Define::addConst(ConstDef *def) {
    def->father = this;
    constDef.push_back(def);
}

void tree::Define::addType(TypeDef *def) {
    def->father = this;
    typeDef.push_back(def);
}

void tree::Define::addVar(VarDef *def) {
    def->father = this;
    varDef.push_back(def);
}

void tree::Define::addFunction(FuncDef *def) {
    def->father = this;
    funcDef.push_back(def);
}
void tree::Program::setBody(Body *body) {
    if (this->body == nullptr) {
        body->father = this;
        this->body   = body;
    }
}

void tree::Body::addStm(Stm *stm) {
    stm->father = this;
    stms.push_back(stm);
}

void tree::Situation::addMatch(Exp *exp) {
    exp->father = this;
    caseVec.push_back(exp);
}

void tree::Situation::addSolution(Body *body) {
    if (solution == nullptr) {
        body->father = this;
        solution     = body;
    }
}

void tree::FuncDef::addArgvs(const std::string &arg_name, Type *arg_type, bool is_formal_parameter) {
    arg_type->father = this;
    argNameVec.push_back(arg_name);
    argTypeVec.push_back(arg_type);
    argFormalVec.push_back(is_formal_parameter);
}

void tree::FuncDef::setReturnType(Type *retType) {
    if (this->retType == nullptr) {
        retType->father = this;
        this->retType   = retType;
    }
}

void tree::FuncDef::setDefination(Define *def) {
    if (define == nullptr) {
        def->father = this;
        define      = def;
    }
}

void tree::FuncDef::setBody(Body *body) {
    if (this->body == nullptr) {
        body->father = this;
        this->body   = body;
    }
}

void tree::CallStm::addArgvs(Exp *exp) {
    exp->father = this;
    this->args.push_back(exp);
}

void tree::IfStm::setCondition(Exp *cond) {
    if (this->condition == nullptr) {
        cond->father = this;
        condition    = cond;
    }
}

void tree::IfStm::addTrue(Body *body) {
    if (this->trueBody == nullptr) {
        this->trueBody = body;
        body->father   = this;
    }
}

void tree::IfStm::addFalse(Body *body) {
    if (this->falseBody == nullptr) {
        this->falseBody         = body;
        this->falseBody->father = this;
    }
}

void tree::CaseStm::addSituation(Situation *situation) {
    situation->father = this;
    situations.push_back(situation);
}

void tree::ForStm::addLoop(Body *body) {
    if (loop == nullptr) {
        loop         = body;
        body->father = this;
    }
}

void tree::WhileStm::addLoop(Body *body) {
    if (loop == nullptr) {
        loop         = body;
        body->father = this;
    }
}

// functions for RepeatStm
void tree::RepeatStm::setCondition(Exp *cond) {
    if (condition == nullptr) {
        condition    = cond;
        cond->father = this;
    }
}

void tree::RepeatStm::addLoop(Body *body) {
    if (loop == nullptr) {
        loop         = body;
        body->father = this;
    }
}

// functions for CallExp
void tree::CallExp::addArgvs(Exp *exp) {
    exp->father = this;
    args.push_back(exp);
}

// -------------------------------------------------

tree::Type *tree::findType(const std::string &typeName, Base *node) {
    switch (node->nodeType) {
        case ND_PROGRAM: return findType(typeName, ((Program *)node)->define);
        case ND_FUNC_DEF: {
            tree::Type *local = findType(typeName, ((FuncDef *)node)->define);
            if (local == nullptr)
                return findType(typeName, node->father);
            else
                return local;
        }
        case ND_DEFINE: {
            Define *d_node = (Define *)node;
            for (TypeDef *iter : d_node->typeDef) {
                Type *result = findType(typeName, iter);
                if (result != nullptr)
                    return result;
            }
            return nullptr;
        }
        case ND_TYPE_DEF: {
            TypeDef *td_node = (TypeDef *)node;
            if (td_node->name == typeName)
                return copyType(td_node->type);
            else
                return nullptr;
        }
        case ND_BODY:
        case ND_SITUATION:
        case ND_LABEL_DEF:
        case ND_CONST_DEF:
        case ND_VAR_DEF:
        case ND_ASSIGN_STM:
        case ND_CALL_STM:
        case ND_CASE_STM:
        case ND_FOR_STM:
        case ND_GOTO_STM:
        case ND_IF_STM:
        case ND_LABEL_STM:
        case ND_REPEAT_STM:
        case ND_WHILE_STM:
        case ND_BINARY_EXP:
        case ND_CALL_EXP:
        case NX_CONST_EXP:
        case ND_UNARY_EXP:
        case ND_VARIABLE_EXP:
        case ND_TYPE: return findType(typeName, node->father);
        default: return nullptr;
    }
}
tree::Type *tree::copyType(Type *origin) {
    auto *copy       = new Type();
    copy->name       = origin->name;
    copy->baseType   = origin->baseType;
    copy->indexStart = origin->indexStart;
    copy->indexEnd   = origin->indexEnd;
    copy->childType.clear();
    for (Type *iter : origin->childType)
        copy->childType.push_back(copyType(iter));
    return copy;
}

bool tree::isSameType(Type *type1, Type *type2) {
    if (type1->baseType == type2->baseType)
        switch (type1->baseType) {
            case TY_INT:
            case TY_CHAR:
            case TY_REAL:
            case TY_BOOL: return true;
            case TY_ARRAY:
                if (type1->indexEnd - type2->indexStart == type2->indexEnd - type2->indexStart)
                    return isSameType(type1->childType[0], type2->childType[0]);
                break;
            case TY_RECORD:
                if (type1->childType.size() == type2->childType.size()) {
                    for (int i = 0; i < type1->childType.size(); i++)
                        if (!isSameType(type1->childType[i], type2->childType[i]))
                            return false;
                    return true;
                }
            default: return false;
        }
    return false;
}

tree::Base *tree::findName(const std::string &name, tree::Base *node) {
    switch (node->nodeType) {
        case ND_PROGRAM: {
            Define *d_node = ((Program *)node)->define;
            for (ConstDef *iter : d_node->constDef)
                if (iter->name == name)
                    return iter;
            for (TypeDef *iter : d_node->typeDef)
                if (iter->name == name)
                    return iter;
            for (VarDef *iter : d_node->varDef)
                if (iter->name == name)
                    return iter;
            for (FuncDef *iter : d_node->funcDef)
                if (iter->name == name)
                    return iter;
            return nullptr;
        }
        case ND_FUNC_DEF: {
            auto *f_node = (FuncDef *)node;
            if (f_node->name == name)
                return f_node;
            for (int i = 0; i < f_node->argNameVec.size(); i++)
                if (f_node->argNameVec[i] == name)
                    return new ArgDef(f_node->argTypeVec[i]);
            Define *d_node = f_node->define;
            for (ConstDef *iter : d_node->constDef)
                if (iter->name == name)
                    return iter;
            for (TypeDef *iter : d_node->typeDef)
                if (iter->name == name)
                    return iter;
            for (VarDef *iter : d_node->varDef)
                if (iter->name == name)
                    return iter;
            for (FuncDef *iter : d_node->funcDef)
                if (iter->name == name)
                    return iter;
            return findName(name, node->father);
        }
        case ND_BODY:
        case ND_SITUATION:
        case ND_LABEL_DEF:
        case ND_CONST_DEF:
        case ND_TYPE_DEF:
        case ND_VAR_DEF:
        case ND_ASSIGN_STM:
        case ND_CALL_STM:
        case ND_CASE_STM:
        case ND_FOR_STM:
        case ND_GOTO_STM:
        case ND_IF_STM:
        case ND_LABEL_STM:
        case ND_REPEAT_STM:
        case ND_WHILE_STM:
        case ND_BINARY_EXP:
        case ND_CALL_EXP:
        case NX_CONST_EXP:
        case ND_DEFINE:
        case ND_UNARY_EXP:
        case ND_VARIABLE_EXP:
        case ND_TYPE: return findName(name, node->father);
        default: return nullptr;
    }
}

bool tree::canFindLabel(const int &label, Base *node) {
    switch (node->nodeType) {
        case ND_PROGRAM: {
            Define *d_node = ((Program *)node)->define;
            for (LabelDef *iter : d_node->labelDef)
                if (iter->labelIndex == label)
                    return true;
            return false;
        }
        case ND_FUNC_DEF: {
            Define *d_node = ((FuncDef *)node)->define;
            for (LabelDef *iter : d_node->labelDef)
                if (iter->labelIndex == label)
                    return true;
            return canFindLabel(label, node->father);
        }
        case ND_BODY:
        case ND_SITUATION:
        case ND_LABEL_DEF:
        case ND_CONST_DEF:
        case ND_TYPE_DEF:
        case ND_VAR_DEF:
        case ND_ASSIGN_STM:
        case ND_CALL_STM:
        case ND_CASE_STM:
        case ND_FOR_STM:
        case ND_GOTO_STM:
        case ND_IF_STM:
        case ND_LABEL_STM:
        case ND_REPEAT_STM:
        case ND_WHILE_STM:
        case ND_BINARY_EXP:
        case ND_CALL_EXP:
        case NX_CONST_EXP:
        case ND_DEFINE:
        case ND_UNARY_EXP:
        case ND_VARIABLE_EXP:
        case ND_TYPE: return canFindLabel(label, node->father);
        default: return false;
    }
}

tree::ConstDef *tree::findConst(const std::string &typeName, Base *node) {
    Base *result = findName(typeName, node);
    if (result == nullptr)
        return nullptr;
    else if (result->nodeType == ND_CONST_DEF)
        return (ConstDef *)result;
    else
        return nullptr;
}

tree::Type *tree::findVar(const std::string &typeName, Base *node) {
    Base *result = findName(typeName, node);
    if (result == nullptr)
        return nullptr;
    else if (result->nodeType == ND_VAR_DEF)
        return ((VarDef *)result)->type;
    else if (result->nodeType == ND_ARG_DEF)
        return ((ArgDef *)result)->type;
    else
        return nullptr;
}

tree::FuncDef *tree::findFunction(const std::string &typeName, Base *node) {
    Base *result = findName(typeName, node);
    if (result == nullptr)
        return nullptr;
    else if (result->nodeType == ND_FUNC_DEF)
        return (FuncDef *)result;
    else
        return nullptr;
}