#ifndef TREE_H
#define TREE_H
#include "symbol.h"
#include <llvm/IR/Value.h>
#include <string>
#include <vector>

class CodeGenContext;

namespace tree {
    // base object
    class Base;
    class Stm;  // without return value
    class Exp;  // with return value

    // block object
    class Program;
    class Routine;
    class Define;
    class Body;
    class Situation;

    // define object
    class LabelDef;
    class ConstDef;
    class TypeDef;
    class VarDef;
    class FunctionDef;
    // stm
    class AssignStm;
    class CallStm;
    class CaseStm;
    class ForStm;
    class GotoStm;
    class IfStm;
    class LabelStm;
    class RepeatStm;
    class WhileStm;
    // exp
    class UnaryExp;
    class BinaryExp;
    class CallExp;
    class ConstantExp;
    class VariableExp;
    // type
    class Type;
    // value
    class Value;

    class Base {
      public:
        int node_type;
        Base *father  = nullptr;
        bool is_legal = true;
        Base(int type = 0) : node_type(type) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) = 0;
        virtual bool checkSemantics()                         = 0;
    };

    class Type : public Base {
      public:
        std::string name;  // use what name to find this value, may be empty
        int base_type;     // 0: int 1: real 2: char 3: bool 5: array 6: record
        int array_start = 0,
            array_end   = 0;  // the index for array. useless if the type is not an array
        std::vector<Type *> child_type;  // a list of the type of children, there is only one
                                         // child if the type is array
        Type() : Base(ND_TYPE) {}
        Type(int _base_type) : Base(ND_TYPE), base_type(_base_type) {}
        llvm::Type *toLLVMType(CodeGenContext &context);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override { return false; }
    };

    class Routine : public Base {
      public:
        Define *define = nullptr;
        Body *body     = nullptr;
        Routine(Define *_define, Body *_body)
            : Base(ND_PROGRAM), define(_define), body(_body) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override { return nullptr; }
        bool checkSemantics() override { return false; }
    };

    class Program : public Base {
      public:
        std::string name;
        Define *define = nullptr;
        Body *body     = nullptr;
        Program(const std::string &_name) : Base(ND_PROGRAM), name(_name) {}
        void addRoutine(Routine *_routine) {
            this->addDefine(_routine->define);
            this->addBody(_routine->body);
        }
        void addDefine(Define *);
        void addBody(Body *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class Define : public Base {
      public:
        std::vector<LabelDef *> label_def;        // can be empty
        std::vector<ConstDef *> const_def;        // can be empty
        std::vector<TypeDef *> type_def;          // can be empty
        std::vector<VarDef *> var_def;            // can be empty
        std::vector<FunctionDef *> function_def;  // can be empty
        Define(std::vector<LabelDef *> _label_def, std::vector<ConstDef *> _const_def,
            std::vector<TypeDef *> _type_def, std::vector<VarDef *> _var_def,
            std::vector<FunctionDef *> _function_def)
            : Base(ND_DEFINE) {
            for (auto ldef : _label_def)
                addLabel(ldef);
            for (auto cdef : _const_def)
                addConst(cdef);
            for (auto tdef : _type_def)
                addType(tdef);
            for (auto vdef : _var_def)
                addVar(vdef);
            for (auto fdef : _function_def)
                addFunction(fdef);
        }
        void addLabel(LabelDef *);
        void addConst(ConstDef *);
        void addType(TypeDef *);
        void addVar(VarDef *);
        void addFunction(FunctionDef *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class Body : public Base {
      public:
        std::vector<Stm *> stms;
        Body() : Base(ND_BODY) {}
        void addStm(Stm *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class Situation : public Base {
      public:
        std::vector<Exp *> match_list;
        Body *solution = nullptr;
        Situation() : Base(ND_SITUATION) {}
        void addMatch(Exp *);
        void addSolution(Body *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class Stm : public Base {
      public:
        Stm(int type = 0) : Base(type) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) = 0;
    };

    class Exp : public Base {
      public:
        Value *return_value = nullptr;
        Type *return_type   = nullptr;
        Exp(int type = 0) : Base(type) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) = 0;
    };

    class LabelDef : public Base {
      public:
        int label_index;
        LabelDef(int _label_index) : Base(ND_LABEL_DEF) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class ConstDef : public Base {
      public:
        std::string name;
        Exp *value = nullptr;  // cannot be nullptr
        ConstDef(const std::string &_name, Exp *_value)
            : Base(ND_CONST_DEF), name(_name), value(_value) {
            _value->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class TypeDef : public Base {
      public:
        std::string name;
        Type *type = nullptr;  // cannot be nullptr
        TypeDef(const std::string &_name, Type *_type)
            : Base(ND_TYPE_DEF), name(_name), type(_type) {
            _type->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class VarDef : public Base {
      public:
        std::string name;
        Type *type     = nullptr;  // cannot be null
        bool is_global = false;
        VarDef(const std::string &_name, Type *_type)
            : Base(ND_VAR_DEF), name(_name), type(_type) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class FunctionDef : public Base {
      public:
        std::string name;
        std::vector<Type *> args_type;
        std::vector<std::string> args_name;
        std::vector<bool> args_is_formal_parameters;  // true:&, false:local
        Type *rtn_type = nullptr;                     // procedure == nullptr
        Define *define = nullptr;
        Body *body     = nullptr;
        FunctionDef(const std::string &_name) : Base(ND_FUNC_DEF), name(_name) {}
        void addArgs(const std::string &, Type *, bool);
        void setReturnType(Type *);
        void addDefine(Define *);
        void addBody(Body *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class ArgDef : public Base {
      public:
        Type *type;
        ArgDef(Type *_type) : Base(ND_ARG_DEF), type(_type) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override { return nullptr; }
        bool checkSemantics() override { return false; }
    };

    class AssignStm : public Stm {
      public:
        Exp *left_value;
        Exp *right_value;
        AssignStm(Exp *left, Exp *right)
            : Stm(ND_ASSIGN_STM), left_value(left), right_value(right) {
            left->father = right->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class CallStm : public Stm {
      public:
        std::string name;
        std::vector<Exp *> args;
        CallStm(const std::string &_name) : Stm(ND_CALL_STM), name(_name) {}
        void addArgs(Exp *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class LabelStm : public Stm {
      public:
        int label;
        LabelStm(const int &_label) : Stm(ND_LABEL_STM), label(_label) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class IfStm : public Stm {
      public:
        Exp *condition = nullptr;
        Body *true_do  = nullptr;  // cannot be nullptr
        Body *false_do = nullptr;  // can be nullptr
        IfStm() : Stm(ND_IF_STM) {}
        void setCondition(Exp *);
        void addTrue(Body *);
        void addFalse(Body *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class CaseStm : public Stm {
      public:
        Exp *object;
        std::vector<Situation *> situations;
        CaseStm(Exp *_object) : Stm(ND_CASE_STM), object(_object) {}
        void addSituation(Situation *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class ForStm : public Stm {
      public:
        std::string iter;
        Exp *start = nullptr, *end = nullptr;
        int step;  // 1 or -1
        Body *loop = nullptr;
        ForStm(const std::string &_iter, Exp *_start, Exp *_end, int _step)
            : Stm(ND_FOR_STM), iter(_iter), start(_start), end(_end), step(_step) {
            _start->father = _end->father = this;
        }
        void addLoop(Body *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class WhileStm : public Stm {
      public:
        Exp *condition = nullptr;
        Body *loop     = nullptr;
        WhileStm(Exp *_condition) : Stm(ND_WHILE_STM), condition(_condition) {
            _condition->father = this;
        }
        void addLoop(Body *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class RepeatStm : public Stm {
      public:
        Exp *condition = nullptr;
        Body *loop     = nullptr;
        RepeatStm() : Stm(ND_REPEAT_STM) {}
        void setCondition(Exp *);
        void addLoop(Body *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class GotoStm : public Stm {
      public:
        int label;
        GotoStm(int _label) : Stm(ND_GOTO_STM), label(_label) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class UnaryExp : public Exp {
      public:
        int op_code;
        Exp *operand;
        UnaryExp(int _op_code, Exp *_operand)
            : Exp(ND_UNARY_EXP), op_code(_op_code), operand(_operand) {
            _operand->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class BinaryExp : public Exp {
      public:
        int op_code;
        Exp *operand1, *operand2;
        BinaryExp(int _op_code, Exp *_operand1, Exp *_operand2)
            : Exp(ND_BINARY_EXP),
              op_code(_op_code),
              operand1(_operand1),
              operand2(_operand2) {
            _operand1->father = operand2->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class CallExp : public Exp {
      public:
        std::string name;
        std::vector<Exp *> args;
        CallExp(const std::string &_name) : Exp(ND_CALL_EXP), name(_name) {}
        void addArgs(Exp *args);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class ConstantExp : public Exp {
      public:
        Value *value;
        ConstantExp(Value *_value) : Exp(NX_CONST_EXP), value(_value) {
            return_value = _value;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class VariableExp : public Exp {
      public:
        std::string name;
        VariableExp(const std::string &_name) : Exp(ND_VARIABLE_EXP), name(_name) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class Value {
      public:
        int base_type;  // 0: int 1: real 2: char 3: boolean 5: array 6: record
        union return_value {
            int integer_value;
            float real_value;
            char char_value;
            bool boolean_value;
            std::string *string_value;
            std::vector<Value *> *children_value;  // a list of the value of children
        } val;
        llvm::Value *codeGen(CodeGenContext *context);
    };

    void printTree(std::string filename, Base *root);
    Type *copyType(Type *origin);
    bool isSameType(Type *type1, Type *type2);
    Base *findName(const std::string &name, Base *node);
    bool canFindLabel(const int &label, Base *node);
    ConstDef *findConst(const std::string &type_name, Base *node);
    Type *findType(const std::string &type_name, Base *node);
    Type *findVar(const std::string &type_name, Base *node);
    FunctionDef *findFunction(const std::string &type_name, Base *node);

}  // namespace tree
#endif
