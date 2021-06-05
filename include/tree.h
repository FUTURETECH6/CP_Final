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
    class FuncDef;
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
        int nodeType;
        Base *father = nullptr;
        bool isLegal = true;
        Base(int type = 0) : nodeType(type) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) = 0;
        virtual bool checkSemantics()                         = 0;
    };

    class Type : public Base {
      public:
        std::string name;  // use what name to find this value, may be empty
        int baseType;      // 0: int 1: real 2: char 3: bool 5: array 6: record
        int indexStart = 0,
            indexEnd   = 0;             // the index for array. useless if the type is not an array
        std::vector<Type *> childType;  // a list of the type of children, there is only one
                                        // child if the type is array
        Type() : Base(ND_TYPE) {}
        Type(int _baseType) : Base(ND_TYPE), baseType(_baseType) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override { return false; }
    };

    class Routine : public Base {
      public:
        Define *define = nullptr;
        Body *body     = nullptr;
        Routine(Define *_define, Body *_body) : Base(ND_PROGRAM), define(_define), body(_body) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override { return nullptr; }
        bool checkSemantics() override { return false; }
    };

    class Program : public Base {
      public:
        std::string name;
        Define *define = nullptr;
        Body *body     = nullptr;
        Program(const std::string &_name) : Base(ND_PROGRAM), name(_name) {}
        void PASCAL_ADD_TURN(Routine *_routine) {
            this->setDefination(_routine->define);
            this->setBody(_routine->body);
        }
        void setDefination(Define *);
        void setBody(Body *);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class Define : public Base {
      public:
        std::vector<LabelDef *> labelDef;  // can be empty
        std::vector<ConstDef *> constDef;  // can be empty
        std::vector<TypeDef *> typeDef;    // can be empty
        std::vector<VarDef *> varDef;      // can be empty
        std::vector<FuncDef *> funcDef;    // can be empty
        Define(std::vector<LabelDef *> _labelDef, std::vector<ConstDef *> _constDef, std::vector<TypeDef *> _typeDef,
            std::vector<VarDef *> _varDef, std::vector<FuncDef *> _funcDef)
            : Base(ND_DEFINE) {
            for (auto ldef : _labelDef)
                addLabel(ldef);
            for (auto cdef : _constDef)
                addConst(cdef);
            for (auto tdef : _typeDef)
                addType(tdef);
            for (auto vdef : _varDef)
                addVar(vdef);
            for (auto fdef : _funcDef)
                addFunction(fdef);
        }
        void addLabel(LabelDef *);
        void addConst(ConstDef *);
        void addType(TypeDef *);
        void addVar(VarDef *);
        void addFunction(FuncDef *);
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
        std::vector<Exp *> caseVec;
        Body *solution = nullptr;
        Situation() : Base(ND_SITUATION) {}
        void addCase(Exp *);
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
        Value *returnVal = nullptr;
        Type *returnType = nullptr;
        Exp(int type = 0) : Base(type) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) = 0;
    };

    class LabelDef : public Base {
      public:
        int labelIndex;
        LabelDef(int _labelIndex) : Base(ND_LABEL_DEF) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class ConstDef : public Base {
      public:
        std::string name;
        Exp *value = nullptr;  // cannot be nullptr
        ConstDef(const std::string &_name, Exp *_value) : Base(ND_CONST_DEF), name(_name), value(_value) {
            _value->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class TypeDef : public Base {
      public:
        std::string name;
        Type *type = nullptr;  // cannot be nullptr
        TypeDef(const std::string &_name, Type *_type) : Base(ND_TYPE_DEF), name(_name), type(_type) {
            _type->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class VarDef : public Base {
      public:
        std::string name;
        Type *type    = nullptr;  // cannot be null
        bool isGlobal = false;
        VarDef(const std::string &_name, Type *_type) : Base(ND_VAR_DEF), name(_name), type(_type) {}
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class FuncDef : public Base {
      public:
        std::string name;
        std::vector<Type *> argTypeVec;
        std::vector<std::string> argNameVec;
        std::vector<bool> argFormalVec;  // whether formal param? true: pass self, false: pass ptr
        Type *retType  = nullptr;        // procedure == nullptr
        Define *define = nullptr;
        Body *body     = nullptr;
        FuncDef(const std::string &_name) : Base(ND_FUNC_DEF), name(_name) {}
        void addArgvs(const std::string &, Type *, bool);
        void setReturnType(Type *);
        void setDefination(Define *);
        void setBody(Body *);
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
        Exp *leftVal;
        Exp *rightVal;
        AssignStm(Exp *left, Exp *right) : Stm(ND_ASSIGN_STM), leftVal(left), rightVal(right) {
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
        void addArgvs(Exp *);
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
        Exp *condition  = nullptr;
        Body *trueBody  = nullptr;  // cannot be nullptr
        Body *falseBody = nullptr;  // can be nullptr
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
        WhileStm(Exp *_condition) : Stm(ND_WHILE_STM), condition(_condition) { _condition->father = this; }
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
        int opcode;
        Exp *operand;
        UnaryExp(int _opcode, Exp *_operand) : Exp(ND_UNARY_EXP), opcode(_opcode), operand(_operand) {
            _operand->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class BinaryExp : public Exp {
      public:
        int opcode;
        Exp *operand1, *operand2;
        BinaryExp(int _opcode, Exp *_operand1, Exp *_operand2)
            : Exp(ND_BINARY_EXP), opcode(_opcode), operand1(_operand1), operand2(_operand2) {
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
        void addArgvs(Exp *args);
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class ConstantExp : public Exp {
      public:
        Value *value;
        ConstantExp(Value *_value) : Exp(NX_CONST_EXP), value(_value) { returnVal = _value; }
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
        int baseType;  // TY_INT, TY_REAL, TY_CHAR, TY_BOOL, TY_STRING, TY_ARRAY, TY_RECORD
        union returnVal {
            int intVal;
            float realVal;
            char charVal;
            bool boolVal;
            std::string *stringVal;
            std::vector<Value *> *childValVec;  // a vector of the value of elements
        } val;
        llvm::Value *codeGen(CodeGenContext *context);
    };

    void visualizeTree(std::string filename, Base *root);
    Type *copyType(Type *origin);
    bool isSameType(Type *type1, Type *type2);
    Base *findName(const std::string &name, Base *node);
    bool canFindLabel(const int &label, Base *node);
    ConstDef *findConst(const std::string &typeName, Base *node);
    Type *findType(const std::string &typeName, Base *node);
    Type *findVar(const std::string &typeName, Base *node);
    FuncDef *findFunction(const std::string &typeName, Base *node);

}  // namespace tree
#endif
