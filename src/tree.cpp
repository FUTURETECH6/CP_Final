#include "tree.h"
#include <fstream>
#include <iostream>

using namespace tree;
using namespace std;

// functions for Program
void Program::addDefine(Define *define) {
    if (this->define == nullptr) {
        define->father = this;
        this->define   = define;
    }
}
// functions for Define
void Define::addLabel(LabelDef *def) {
    def->father = this;
    label_def.push_back(def);
}

void Define::addConst(ConstDef *def) {
    def->father = this;
    const_def.push_back(def);
}

void Define::addType(TypeDef *def) {
    def->father = this;
    type_def.push_back(def);
}

void Define::addVar(VarDef *def) {
    def->father = this;
    var_def.push_back(def);
}

void Define::addFunction(FunctionDef *def) {
    def->father = this;
    function_def.push_back(def);
}
void Program::addBody(Body *body) {
    if (this->body == nullptr) {
        body->father = this;
        this->body   = body;
    }
}

void Body::addStm(Stm *stm) {
    stm->father = this;
    stms.push_back(stm);
}

void Situation::addMatch(Exp *exp) {
    exp->father = this;
    match_list.push_back(exp);
}

void Situation::addSolution(Body *body) {
    if (solution == nullptr) {
        body->father = this;
        solution     = body;
    }
}

void FunctionDef::addArgs(
    const std::string &arg_name, Type *arg_type, bool is_formal_parameter) {
    arg_type->father = this;
    args_name.push_back(arg_name);
    args_type.push_back(arg_type);
    args_is_formal_parameters.push_back(is_formal_parameter);
}

void FunctionDef::setReturnType(Type *rtn_type) {
    if (this->rtn_type == nullptr) {
        rtn_type->father = this;
        this->rtn_type   = rtn_type;
    }
}

void FunctionDef::addDefine(Define *def) {
    if (define == nullptr) {
        def->father = this;
        define      = def;
    }
}

void FunctionDef::addBody(Body *body) {
    if (this->body == nullptr) {
        body->father = this;
        this->body   = body;
    }
}

void CallStm::addArgs(Exp *exp) {
    exp->father = this;
    this->args.push_back(exp);
}

void IfStm::setCondition(Exp *cond) {
    if (this->condition == nullptr) {
        cond->father = this;
        condition    = cond;
    }
}

void IfStm::addTrue(Body *body) {
    if (this->true_do == nullptr) {
        this->true_do = body;
        body->father  = this;
    }
}

void IfStm::addFalse(Body *body) {
    if (this->false_do == nullptr) {
        this->false_do         = body;
        this->false_do->father = this;
    }
}

void CaseStm::addSituation(Situation *situation) {
    situation->father = this;
    situations.push_back(situation);
}

void ForStm::addLoop(Body *body) {
    if (loop == nullptr) {
        loop         = body;
        body->father = this;
    }
}

void WhileStm::addLoop(Body *body) {
    if (loop == nullptr) {
        loop         = body;
        body->father = this;
    }
}

// functions for RepeatStm
void RepeatStm::setCondition(Exp *cond) {
    if (condition == nullptr) {
        condition    = cond;
        cond->father = this;
    }
}

void RepeatStm::addLoop(Body *body) {
    if (loop == nullptr) {
        loop         = body;
        body->father = this;
    }
}

// functions for CallExp
void CallExp::addArgs(Exp *exp) {
    exp->father = this;
    args.push_back(exp);
}

std ::string print_rec(Value *value, int layer) {
    std::string str;
    switch (value->base_type) {
        case TY_INTEGER: {
            char val[10];
            sprintf(val, "%d", value->val.integer_value);
            str.append(val);
        } break;
        case TY_REAL: {
            char val[10];
            sprintf(val, "%.2f", value->val.real_value);
            str.append(val);
        } break;
        case TY_CHAR: {
            char val[2];
            sprintf(val, "%c", value->val.char_value);
            str.append(val);
        } break;
        case TY_BOOLEAN: {
            str.append(value->val.boolean_value ? "true" : "false");
        } break;
        case TY_ARRAY:
        case TY_RECORD: {
            for (auto child : *value->val.children_value)
                str.append(print_rec(child, layer + 1));
        }
        default: {
            str.append("\"There is something wrong. The type is unrecognised.\"");
        }
    }
    return str;
}

std::string print_tab(int layer) {
    std::string str;
    for (int i = 0; i < layer; i++)
        str.append("│    ");
    str.append("├────");
    return str;
}

std::string print_tab1(int layer) {
    std::string str;
    for (int i = 0; i < layer; i++)
        str.append("│    ");
    str.append("└────");
    return str;
}

std::string print_tab2(int layer) {
    std::string str;
    for (int i = 0; i < layer - 1; i++)
        str.append("│    ");
    str.append("     ├────");
    return str;
}
std::string print_tab3(int layer) {
    std::string str;
    for (int i = 0; i < layer - 1; i++)
        str.append("│    ");
    str.append("     └────");
    return str;
}

std::string print_rec(Base *ori_node, int layer, bool noNext = true) {
    std::string str;
    if (ori_node == nullptr)
        str = "NULL";
    else {
        switch (ori_node->node_type) {
            case N_PROGRAM: {
                auto *node = (Program *)ori_node;
                str.append("Program: \n");
                str.append(print_tab(layer));
                str.append("Name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab(layer));
                str.append("Define: ");
                str.append(print_rec(node->define, layer + 1));
                str.append("\n");
                str.append(print_tab1(layer));
                str.append("Body: ");
                str.append(print_rec(node->body, layer + 1));
            } break;

            case N_DEFINE: {
                auto *node = (Define *)ori_node;
                if (node->const_def.size() > 0) {
                    for (auto stm : node->const_def) {
                        str.append("\n");
                        if (node->type_def.size() > 0 || node->var_def.size() > 0 ||
                            node->function_def.size() > 0)
                            str.append(print_tab(layer));
                        else
                            str.append(print_tab1(layer));
                        str.append("const: ");
                        str.append(print_rec(stm, layer + 1));
                    }
                }
                if (node->type_def.size() > 0) {
                    for (auto stm : node->type_def) {
                        str.append("\n");
                        if (node->var_def.size() > 0 || node->function_def.size() > 0)
                            str.append(print_tab(layer));
                        else
                            str.append(print_tab1(layer));
                        str.append("type: ");
                        str.append(print_rec(stm, layer + 1));
                    }
                }
                if (node->var_def.size() > 0) {
                    for (auto stm : node->var_def) {
                        str.append("\n");
                        if (node->function_def.size() > 0)
                            str.append(print_tab(layer));
                        else
                            str.append(print_tab1(layer));
                        str.append("var: ");
                        str.append(print_rec(stm, layer + 1));
                    }
                }
                if (node->function_def.size() > 0) {
                    for (int i = 0; i < node->function_def.size(); i++) {
                        str.append("\n");
                        str.append(print_tab1(layer));
                        str.append("function: ");
                        if (i != node->function_def.size())
                            str.append(print_rec(node->function_def[i], layer + 1));
                        else
                            str.append(print_rec(node->function_def[i], layer + 1, false));
                    }
                }
            } break;

            case N_BODY: {
                auto *node = (Body *)ori_node;
                for (auto stm : node->stms) {
                    str.append(print_rec(stm, layer));
                }
            } break;

            case N_SITUATION: {
                auto *node = (Situation *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("match_list: ");
                for (auto match_item : node->match_list)
                    str.append(print_rec(match_item, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("to_do: ");
                str.append(print_rec(node->solution, layer + 1));
            } break;

                // case N_LABEL_DEF: {}
                // break;

            case N_CONST_DEF: {
                auto *node = (ConstDef *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("const_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab1(layer));  // modified
                str.append("const_value: ");
                str.append(print_rec(node->value, layer + 1, false));
            } break;

            case N_TYPE_DEF: {
                auto *node = (TypeDef *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("type_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab(layer));
                str.append("structure: ");
                str.append(print_rec(node->type, layer + 1));
            } break;

            case N_VAR_DEF: {
                auto *node = (VarDef *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("var_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab1(layer));
                str.append("structure: ");
                str.append(print_rec(node->type, layer + 1));
            } break;

            case N_FUNCTION_DEF: {
                auto *node = (FunctionDef *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("func_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab(layer));
                str.append("args: ");
                for (int i = 0; i < node->args_name.size(); i++) {
                    str.append("\n");
                    str.append(print_tab(layer));
                    str.append("var_name: ");
                    str.append(node->args_name[i]);
                    str.append("\n");
                    str.append(print_tab(layer));
                    str.append("arg_is_formal_parameter: ");
                    str.append(print_rec(node->args_type[i], layer + 1));
                    str.append(node->args_is_formal_parameters[i] ? " true" : " false");
                }
                if (node->rtn_type != nullptr) {
                    str.append("\n");
                    str.append(print_tab1(layer));
                    str.append("return_type: ");
                    str.append(print_rec(node->rtn_type, layer + 1));
                }
                if (node->define != nullptr) {
                    str.append("\n");
                    str.append(print_tab(layer));
                    str.append("defines: ");
                    str.append(print_rec(node->define, layer + 1));
                }
                str.append("\n");
                str.append(print_tab(layer));
                str.append("body: ");
                str.append(print_rec(node->body, layer + 1));
            } break;

            case N_ASSIGN_STM: {
                auto *node = (AssignStm *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("left: ");
                str.append(print_rec(node->left_value, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("right: ");
                str.append(print_rec(node->right_value, layer + 1));

            } break;

            case N_CALL_STM: {
                auto *node = (CallStm *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("proc: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab(layer));
                str.append("args: ");
                for (auto arg : node->args)
                    str.append(print_rec(arg, layer + 1));
            } break;

            case N_CASE_STM: {
                auto *node = (CaseStm *)ori_node;
                str.append(print_tab(layer));
                str.append("switch_item: ");
                str.append(print_rec(node->object, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("situations: ");
                for (auto situation : node->situations)
                    str.append(print_rec(situation, layer + 1));

            } break;

            case N_FOR_STM: {
                auto *node = (ForStm *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("iter: ");
                str.append(node->iter);
                str.append("\n");
                str.append(print_tab(layer));
                str.append("start: ");
                str.append(print_rec(node->start, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("end: ");
                str.append(print_rec(node->end, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("step: ");
                if (node->step == 1)
                    str.append("1");
                else
                    str.append("-1");
                str.append("\n");
                str.append(print_tab(layer));
                str.append("body: ");
                str.append(print_rec(node->loop, layer + 1));
            } break;

                // case N_GOTO_STM: {}
                // break;

            case N_IF_STM: {
                auto *node = (IfStm *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("condition: ");
                str.append(print_rec(node->condition, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("true_body: ");
                str.append(print_rec(node->true_do, layer + 1));
                if (node->false_do != nullptr) {
                    str.append("\n");
                    str.append(print_tab(layer));
                    str.append("false_body: ");
                    str.append(print_rec(node->false_do, layer + 1));
                }
            } break;

                // case N_LABEL_STM: {
                //     str.append(print_tab(layer));
                // }
                // break;

            case N_REPEAT_STM: {
                auto *node = (RepeatStm *)ori_node;
                // str.append("\n");
                str.append(print_tab(layer));
                str.append("body: ");
                str.append(print_rec(node->loop, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("condition: ");
                str.append(print_rec(node->condition, layer + 1));
            } break;

            case N_WHILE_STM: {
                auto *node = (WhileStm *)ori_node;
                // str.append("\n");
                str.append(print_tab(layer));
                str.append("condition: ");
                str.append(print_rec(node->condition, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("body: ");
                str.append(print_rec(node->loop, layer + 1));
            } break;

            case N_BINARY_EXP: {
                auto *node = (BinaryExp *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("bin_op: ");
                str.append(getOpNameByID(node->op_code));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("operand 1: ");
                str.append(print_rec(node->operand1, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("operand 2: ");
                str.append(print_rec(node->operand2, layer + 1));
                str.append("\n");
                str.append(print_tab1(layer));
                str.append("return_type: ");
                str.append(print_rec(node->return_type, layer + 1));
            } break;

            case N_CALL_EXP: {
                auto *node = (CallExp *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("func_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab(layer));
                str.append("args: ");
                for (auto arg : node->args)
                    str.append(print_rec(arg, layer + 1));
                str.append("\n");
                str.append(print_tab1(layer));
                str.append("return_type: ");
                str.append(print_rec(node->return_type, layer + 1));
            } break;

            case N_CONSTANT_EXP: {
                auto *node = (ConstantExp *)ori_node;
                str.append("\n");
                if (noNext == false)
                    str.append(print_tab2(layer));
                else
                    str.append(print_tab(layer));
                str.append("const_value: ");
                str.append(print_rec(node->value, layer + 1));
                str.append("\n");
                if (noNext == false)
                    str.append(print_tab3(layer));
                else
                    str.append(print_tab1(layer));
                str.append("return_type: ");
                str.append(print_rec(node->return_type, layer + 1));
            } break;

            case N_UNARY_EXP: {
                auto *node = (UnaryExp *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("una_op: ");
                str.append(getOpNameByID(node->op_code));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("operand: ");
                str.append(print_rec(node->operand, layer + 1));
                str.append("\n");
                str.append(print_tab1(layer));
                str.append("return_type: ");
                str.append(print_rec(node->return_type, layer + 1));
            } break;

            case N_VARIABLE_EXP: {
                auto *node = (VariableExp *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("var_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab1(layer));
                str.append("return_type: ");
                str.append(print_rec(node->return_type, layer + 1));
            } break;

            case N_TYPE: {
                auto *node = (Type *)ori_node;
                switch (node->base_type) {
                    case TY_INTEGER: {
                        str.append("integer");
                    } break;
                    case TY_REAL: {
                        str.append("real");
                    } break;
                    case TY_CHAR: {
                        str.append("char");
                    } break;
                    case TY_BOOLEAN: {
                        str.append("boolean");
                    } break;
                    case TY_ARRAY: {
                        str.append("\"type\":\"array\", \"start_index\": ");
                        char num[100];
                        sprintf(num, "%d", node->array_start);
                        str.append(num);
                        str.append(", \"end_index\": ");
                        sprintf(num, "%d", node->array_end);
                        str.append(num);
                        str.append(", \"child_type\": ");
                        str.append(print_rec(node->child_type[0], layer + 1));
                    } break;
                    case TY_RECORD: {
                        str.append(print_tab(layer));
                        str.append("\"type\":\"record\",\"child_type\": ");
                        for (auto child : node->child_type) {
                            str.append("\n");
                            str.append(print_tab(layer));
                            str.append("name: ");
                            str.append(child->name);
                            str.append("\n");
                            str.append(print_tab(layer));
                            str.append("type: ");
                            str.append(print_rec(child, layer + 1));
                        }
                    } break;

                    default: {
                        str.append(
                            "\"There is something wrong. The type cannot be recognised.\"");
                    }
                }
            } break;
            default: {
                str.append("\"There is something wrong. The node is unrecognised.\"");
            }
        }
    }
    return str;
}

void tree::printTree(std::string filename, Base *root) {
    std::string str = print_rec(root, 0);
    std::ofstream SaveFile(filename + ".t");
    SaveFile << str;
    SaveFile.close();
}

Type *tree::findType(const std::string &type_name, Base *node) {
    switch (node->node_type) {
        case N_PROGRAM: return findType(type_name, ((Program *)node)->define);
        case N_FUNCTION_DEF: {
            Type *local = findType(type_name, ((FunctionDef *)node)->define);
            if (local == nullptr)
                return findType(type_name, node->father);
            else
                return local;
        }
        case N_DEFINE: {
            Define *d_node = (Define *)node;
            for (TypeDef *iter : d_node->type_def) {
                Type *result = findType(type_name, iter);
                if (result != nullptr)
                    return result;
            }
            return nullptr;
        }
        case N_TYPE_DEF: {
            TypeDef *td_node = (TypeDef *)node;
            if (td_node->name == type_name)
                return copyType(td_node->type);
            else
                return nullptr;
        }
        case N_BODY:
        case N_SITUATION:
        case N_LABEL_DEF:
        case N_CONST_DEF:
        case N_VAR_DEF:
        case N_ASSIGN_STM:
        case N_CALL_STM:
        case N_CASE_STM:
        case N_FOR_STM:
        case N_GOTO_STM:
        case N_IF_STM:
        case N_LABEL_STM:
        case N_REPEAT_STM:
        case N_WHILE_STM:
        case N_BINARY_EXP:
        case N_CALL_EXP:
        case N_CONSTANT_EXP:
        case N_UNARY_EXP:
        case N_VARIABLE_EXP:
        case N_TYPE: return findType(type_name, node->father);
        default: return nullptr;
    }
}
Type *tree::copyType(Type *origin) {
    auto *copy        = new Type();
    copy->name        = origin->name;
    copy->base_type   = origin->base_type;
    copy->array_start = origin->array_start;
    copy->array_end   = origin->array_end;
    copy->child_type.clear();
    for (Type *iter : origin->child_type)
        copy->child_type.push_back(copyType(iter));
    return copy;
}

bool tree::isSameType(Type *type1, Type *type2) {
    if (type1->base_type == type2->base_type)
        switch (type1->base_type) {
            case TY_INTEGER:
            case TY_CHAR:
            case TY_REAL:
            case TY_BOOLEAN: return true;
            case TY_ARRAY:
                if (type1->array_end - type2->array_start ==
                    type2->array_end - type2->array_start)
                    return isSameType(type1->child_type[0], type2->child_type[0]);
                break;
            case TY_RECORD:
                if (type1->child_type.size() == type2->child_type.size()) {
                    for (int i = 0; i < type1->child_type.size(); i++)
                        if (!isSameType(type1->child_type[i], type2->child_type[i]))
                            return false;
                    return true;
                }
            default: return false;
        }
    return false;
}

Base *tree::findName(const std::string &name, tree::Base *node) {
    switch (node->node_type) {
        case N_PROGRAM: {
            Define *d_node = ((Program *)node)->define;
            for (ConstDef *iter : d_node->const_def)
                if (iter->name == name)
                    return iter;
            for (TypeDef *iter : d_node->type_def)
                if (iter->name == name)
                    return iter;
            for (VarDef *iter : d_node->var_def)
                if (iter->name == name)
                    return iter;
            for (FunctionDef *iter : d_node->function_def)
                if (iter->name == name)
                    return iter;
            return nullptr;
        }
        case N_FUNCTION_DEF: {
            auto *f_node = (FunctionDef *)node;
            if (f_node->name == name)
                return f_node;
            for (int i = 0; i < f_node->args_name.size(); i++)
                if (f_node->args_name[i] == name)
                    return new ArgDef(f_node->args_type[i]);
            Define *d_node = f_node->define;
            for (ConstDef *iter : d_node->const_def)
                if (iter->name == name)
                    return iter;
            for (TypeDef *iter : d_node->type_def)
                if (iter->name == name)
                    return iter;
            for (VarDef *iter : d_node->var_def)
                if (iter->name == name)
                    return iter;
            for (FunctionDef *iter : d_node->function_def)
                if (iter->name == name)
                    return iter;
            return findName(name, node->father);
        }
        case N_BODY:
        case N_SITUATION:
        case N_LABEL_DEF:
        case N_CONST_DEF:
        case N_TYPE_DEF:
        case N_VAR_DEF:
        case N_ASSIGN_STM:
        case N_CALL_STM:
        case N_CASE_STM:
        case N_FOR_STM:
        case N_GOTO_STM:
        case N_IF_STM:
        case N_LABEL_STM:
        case N_REPEAT_STM:
        case N_WHILE_STM:
        case N_BINARY_EXP:
        case N_CALL_EXP:
        case N_CONSTANT_EXP:
        case N_DEFINE:
        case N_UNARY_EXP:
        case N_VARIABLE_EXP:
        case N_TYPE: return findName(name, node->father);
        default: return nullptr;
    }
}

bool tree::canFindLabel(const int &label, Base *node) {
    switch (node->node_type) {
        case N_PROGRAM: {
            Define *d_node = ((Program *)node)->define;
            for (LabelDef *iter : d_node->label_def)
                if (iter->label_index == label)
                    return true;
            return false;
        }
        case N_FUNCTION_DEF: {
            Define *d_node = ((FunctionDef *)node)->define;
            for (LabelDef *iter : d_node->label_def)
                if (iter->label_index == label)
                    return true;
            return canFindLabel(label, node->father);
        }
        case N_BODY:
        case N_SITUATION:
        case N_LABEL_DEF:
        case N_CONST_DEF:
        case N_TYPE_DEF:
        case N_VAR_DEF:
        case N_ASSIGN_STM:
        case N_CALL_STM:
        case N_CASE_STM:
        case N_FOR_STM:
        case N_GOTO_STM:
        case N_IF_STM:
        case N_LABEL_STM:
        case N_REPEAT_STM:
        case N_WHILE_STM:
        case N_BINARY_EXP:
        case N_CALL_EXP:
        case N_CONSTANT_EXP:
        case N_DEFINE:
        case N_UNARY_EXP:
        case N_VARIABLE_EXP:
        case N_TYPE: return canFindLabel(label, node->father);
        default: return false;
    }
}

ConstDef *tree::findConst(const std::string &type_name, Base *node) {
    Base *result = findName(type_name, node);
    if (result == nullptr)
        return nullptr;
    else if (result->node_type == N_CONST_DEF)
        return (ConstDef *)result;
    else
        return nullptr;
}

Type *tree::findVar(const std::string &type_name, Base *node) {
    Base *result = findName(type_name, node);
    if (result == nullptr)
        return nullptr;
    else if (result->node_type == N_VAR_DEF)
        return ((VarDef *)result)->type;
    else if (result->node_type == N_ARG_DEF)
        return ((ArgDef *)result)->type;
    else
        return nullptr;
}

FunctionDef *tree::findFunction(const std::string &type_name, Base *node) {
    Base *result = findName(type_name, node);
    if (result == nullptr)
        return nullptr;
    else if (result->node_type == N_FUNCTION_DEF)
        return (FunctionDef *)result;
    else
        return nullptr;
}
