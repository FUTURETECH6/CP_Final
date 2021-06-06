#ifndef TREE_H
#define TREE_H
#include "symbol.h"
#include <llvm/IR/Value.h>
#include <string>
#include <vector>

class CodeGenContext;

namespace tree {
    /* Base */
    class Base;
    class Stm; /* objects that don't return values */
    class Exp; /* objects that return values */

    /* Stm */
    class AssignStm;
    class IfStm;
    class ForStm;
    class WhileStm;
    class GotoStm;
    class LabelStm;
    class RepeatStm;
    class CallStm;
    class CaseStm;

    /* Exp */
    class UnaryExp;
    class BinaryExp;
    class ConstantExp;
    class VariableExp;
    class CallExp;

    /* Block */
    class Program;
    class Routine;
    class Define;
    class Body;
    class Situation;

    /* Define */
    class TypeDef;
    class VarDef;
    class LabelDef;
    class ConstDef;
    class FuncDef;

    /* Type */
    class Type;

    /* Value */
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
        std::string name; /* Name used to search the value */
        int baseType;
        int indexStart = 0, indexEnd = 0; /* Array index */
        std::vector<Type *> childType;    /* Type of children */

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
        /* The followings can be null */
        std::vector<LabelDef *> labelDef;
        std::vector<ConstDef *> constDef;
        std::vector<TypeDef *> typeDef;
        std::vector<VarDef *> varDef;
        std::vector<FuncDef *> funcDef;
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
        Exp *value = nullptr; /* Err if value is null */
        ConstDef(const std::string &_name, Exp *_value) : Base(ND_CONST_DEF), name(_name), value(_value) {
            _value->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class TypeDef : public Base {
      public:
        std::string name;
        Type *type = nullptr; /* Err if type is null */
        TypeDef(const std::string &_name, Type *_type) : Base(ND_TYPE_DEF), name(_name), type(_type) {
            _type->father = this;
        }
        virtual llvm::Value *codeGen(CodeGenContext *context) override;
        bool checkSemantics() override;
    };

    class VarDef : public Base {
      public:
        std::string name;
        Type *type    = nullptr; /* Err if type is null */
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
        std::vector<bool> argFormalVec; /* Pass self for formal parameters, otherwise pass ptr */
        Type *retType  = nullptr;       /* Return null for procedures*/
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
        Body *trueBody  = nullptr; /* Err if trueBody is null */
        Body *falseBody = nullptr; /* Falsebody can be null */
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
        int step; /* Possible value : 1 / -1 */
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
        Exp *OPRFIRST, *OPRSECOND;
        BinaryExp(int _opcode, Exp *_OPRFIRST, Exp *_OPRSECOND)
            : Exp(ND_BINARY_EXP), opcode(_opcode), OPRFIRST(_OPRFIRST), OPRSECOND(_OPRSECOND) {
            _OPRFIRST->father = OPRSECOND->father = this;
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
        int baseType; /* Possible value : int(0), real(1), char(2), bool(3), string(4), array(5), record(6) */
        union returnVal {
            int intVal;
            float realVal;
            char charVal;
            bool boolVal;
            std::string *stringVal;
            std::vector<Value *> *childValVec; /* Values of children */
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
