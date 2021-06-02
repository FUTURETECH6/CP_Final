#include "tree.h"
#include <fstream>
#include <iostream>

std::string print_rec(tree::Value *value, int layer) {
    std::string str;
    switch (value->baseType) {
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

std::string print_rec(tree::Base *ori_node, int layer, bool noNext = true) {
    std::string str;
    if (ori_node == nullptr)
        str = "NULL";
    else {
        switch (ori_node->node_type) {
            case ND_PROGRAM: {
                auto *node = (tree::Program *)ori_node;
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

            case ND_DEFINE: {
                auto *node = (tree::Define *)ori_node;
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

            case ND_BODY: {
                auto *node = (tree::Body *)ori_node;
                for (auto stm : node->stms) {
                    str.append(print_rec(stm, layer));
                }
            } break;

            case ND_SITUATION: {
                auto *node = (tree::Situation *)ori_node;
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

                // case ND_LABEL_DEF: break;

            case ND_CONST_DEF: {
                auto *node = (tree::ConstDef *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("const_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab1(layer));  // modified
                str.append("const_value: ");
                str.append(print_rec(node->value, layer + 1, false));
            } break;

            case ND_TYPE_DEF: {
                auto *node = (tree::TypeDef *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("type_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab(layer));
                str.append("structure: ");
                str.append(print_rec(node->type, layer + 1));
            } break;

            case ND_VAR_DEF: {
                auto *node = (tree::VarDef *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("var_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab1(layer));
                str.append("structure: ");
                str.append(print_rec(node->type, layer + 1));
            } break;

            case ND_FUNC_DEF: {
                auto *node = (tree::FunctionDef *)ori_node;
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

            case ND_ASSIGN_STM: {
                auto *node = (tree::AssignStm *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("left: ");
                str.append(print_rec(node->left_value, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("right: ");
                str.append(print_rec(node->right_value, layer + 1));

            } break;

            case ND_CALL_STM: {
                auto *node = (tree::CallStm *)ori_node;
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

            case ND_CASE_STM: {
                auto *node = (tree::CaseStm *)ori_node;
                str.append(print_tab(layer));
                str.append("switch_item: ");
                str.append(print_rec(node->object, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("situations: ");
                for (auto situation : node->situations)
                    str.append(print_rec(situation, layer + 1));

            } break;

            case ND_FOR_STM: {
                auto *node = (tree::ForStm *)ori_node;
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

                // case ND_GOTO_STM: break;

            case ND_IF_STM: {
                auto *node = (tree::IfStm *)ori_node;
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

            case ND_LABEL_STM: str.append(print_tab(layer)); break;

            case ND_REPEAT_STM: {
                auto *node = (tree::RepeatStm *)ori_node;
                // str.append("\n");
                str.append(print_tab(layer));
                str.append("body: ");
                str.append(print_rec(node->loop, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("condition: ");
                str.append(print_rec(node->condition, layer + 1));
            } break;

            case ND_WHILE_STM: {
                auto *node = (tree::WhileStm *)ori_node;
                // str.append("\n");
                str.append(print_tab(layer));
                str.append("condition: ");
                str.append(print_rec(node->condition, layer + 1));
                str.append("\n");
                str.append(print_tab(layer));
                str.append("body: ");
                str.append(print_rec(node->loop, layer + 1));
            } break;

            case ND_BINARY_EXP: {
                auto *node = (tree::BinaryExp *)ori_node;
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

            case ND_CALL_EXP: {
                auto *node = (tree::CallExp *)ori_node;
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

            case NX_CONST_EXP: {
                auto *node = (tree::ConstantExp *)ori_node;
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

            case ND_UNARY_EXP: {
                auto *node = (tree::UnaryExp *)ori_node;
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

            case ND_VARIABLE_EXP: {
                auto *node = (tree::VariableExp *)ori_node;
                str.append("\n");
                str.append(print_tab(layer));
                str.append("var_name: ");
                str.append(node->name);
                str.append("\n");
                str.append(print_tab1(layer));
                str.append("return_type: ");
                str.append(print_rec(node->return_type, layer + 1));
            } break;

            case ND_TYPE: {
                auto *node = (tree::Type *)ori_node;
                switch (node->baseType) {
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
                        str.append(", \"childType\": ");
                        str.append(print_rec(node->childType[0], layer + 1));
                    } break;
                    case TY_RECORD: {
                        str.append(print_tab(layer));
                        str.append("\"type\":\"record\",\"childType\": ");
                        for (auto child : node->childType) {
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
    std::ofstream SaveFile(filename);
    SaveFile << str;
    SaveFile.close();
}