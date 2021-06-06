#ifndef TREE_H
#define TREE_H
#include "symbol.h"
#include <llvm/IR/Value.h>
#include <string>
#include <vector>

class ContextOfCodeCreate;

namespace tree {
    /* Base */
    class Base;
    class Stm; /* objects that don't return values */
    class Exp; /* objects that return values */

    /* Clause */
    class StatementAssign; /* assignment clause */
    class IfStm;           /* select clause */
    class ForStm;          /* for clause */
    class WhileStm;        /* while clause */
    class GotoStm;         /* goto clause */
    class LabelStm;        /* label clause */
    class StatementRepeat; /* repeat clause */
    class CallStm;         /* call function */
    class CaseStm;         /* case clause */

    /* Expression */
    class UnaryExp;        /* Unary operator expression */
    class BinaryExp;       /* binary operator expression */
    class EXPRESSIONConst; /* constant expression */
    class VariableExp;     /* id expression */
    class CallExp;         /* function call expression */

    /* Block */
    class Program; /* root of code */
    class Routine;
    class Define;    /* root of definitions */
    class Body;      /* root of sentence block */
    class Situation; /* root of case clause */

    /* Define */
    class TypeDef;  /* define a single type */
    class VarDef;   /* define a variable */
    class LabelDef; /* define a label */
    class ConstDef; /* define a constant */
    class FuncDef;  /* define a function */

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
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) = 0;
        virtual bool SEMANT_CHECK_LEGAL()                                   = 0;
    };

    class Type : public Base {
      public:
        std::string name; /* Name used to search the value */
        int baseType;
        int indexStart = 0, indexEnd = 0; /* Array index */
        std::vector<Type *> childType;    /* Type of children */

        Type() : Base(ND_TYPE) {}
        Type(int _baseType) : Base(ND_TYPE), baseType(_baseType) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override { return false; }
    };

    class Routine : public Base {
      public:
        Define *define = nullptr;
        Body *body     = nullptr;
        Routine(Define *_define, Body *_body) : Base(ND_PROGRAM), define(_define), body(_body) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override { return nullptr; }
        bool SEMANT_CHECK_LEGAL() override { return false; }
    };

    class Program : public Base {
      public:
        std::string name;
        Define *define = nullptr;
        Body *body     = nullptr;
        Program(const std::string &_name) : Base(ND_PROGRAM), name(_name) {}
        void PASCAL_ADD_TURN(Routine *_routine) {
            this->DefSetup(_routine->define);
            this->setBody(_routine->body);
        }
        void DefSetup(Define *);
        void setBody(Body *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
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
                CONSTPLUS(cdef);
            for (auto tdef : _typeDef)
                addType(tdef);
            for (auto vdef : _varDef)
                addVar(vdef);
            for (auto fdef : _funcDef)
                addFunction(fdef);
        }
        void addLabel(LabelDef *);
        void CONSTPLUS(ConstDef *);
        void addType(TypeDef *);
        void addVar(VarDef *);
        void addFunction(FuncDef *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class Body : public Base {
      public:
        std::vector<Stm *> stms;
        Body() : Base(ND_BODY) {}
        void StatementAdd(Stm *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class Situation : public Base {
      public:
        std::vector<Exp *> caseVec;
        Body *solution = nullptr;
        Situation() : Base(ND_SITUATION) {}
        void addCase(Exp *);
        void SolutionAdd(Body *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class Stm : public Base {
      public:
        Stm(int type = 0) : Base(type) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) = 0;
    };

    class Exp : public Base {
      public:
        Value *returnVal = nullptr;
        Type *returnType = nullptr;
        Exp(int type = 0) : Base(type) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) = 0;
    };

    class LabelDef : public Base {
      public:
        int labelIndex;
        LabelDef(int _labelIndex) : Base(ND_LABEL_DEF) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class ConstDef : public Base {
      public:
        std::string name;
        Exp *value = nullptr; /* Err if value is null */
        ConstDef(const std::string &_name, Exp *_value) : Base(ND_CONST_DEF), name(_name), value(_value) {
            _value->father = this;
        }
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class TypeDef : public Base {
      public:
        std::string name;
        Type *type = nullptr; /* Err if type is null */
        TypeDef(const std::string &_name, Type *_type) : Base(ND_TYPE_DEF), name(_name), type(_type) {
            _type->father = this;
        }
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class VarDef : public Base {
      public:
        std::string name;
        Type *type    = nullptr; /* Err if type is null */
        bool isGlobal = false;
        VarDef(const std::string &_name, Type *_type) : Base(ND_VAR_DEF), name(_name), type(_type) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class FuncDef : public Base {
      public:
        std::string name;
        std::vector<Type *> TypeofVectorPara;
        std::vector<std::string> VectorNamePara;
        std::vector<bool> FomalVectorPara; /* Pass self for formal parameters, otherwise pass ptr */
        Type *retType  = nullptr;          /* Return null for procedures*/
        Define *define = nullptr;
        Body *body     = nullptr;
        FuncDef(const std::string &_name) : Base(ND_FUNC_DEF), name(_name) {}
        void PARAMAdd(const std::string &, Type *, bool);
        void setReturnType(Type *);
        void DefSetup(Define *);
        void setBody(Body *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class ArgDef : public Base {
      public:
        Type *type;
        ArgDef(Type *_type) : Base(ND_ARG_DEF), type(_type) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override { return nullptr; }
        bool SEMANT_CHECK_LEGAL() override { return false; }
    };

    class StatementAssign : public Stm {
      public:
        Exp *leftVal;
        Exp *rightVal;
        StatementAssign(Exp *left, Exp *right) : Stm(ND_ASSIGN_STM), leftVal(left), rightVal(right) {
            left->father = right->father = this;
        }
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class CallStm : public Stm {
      public:
        std::string name;
        std::vector<Exp *> args;
        CallStm(const std::string &_name) : Stm(ND_CALL_STM), name(_name) {}
        void PARAMAdd(Exp *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class LabelStm : public Stm {
      public:
        int label;
        LabelStm(const int &_label) : Stm(ND_LABEL_STM), label(_label) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class IfStm : public Stm {
      public:
        Exp *condition  = nullptr;
        Body *trueBody  = nullptr; /* Err if trueBody is null */
        Body *falseBody = nullptr; /* Falsebody can be null */
        IfStm() : Stm(ND_IF_STM) {}
        void ConditionSetup(Exp *);
        void TrueAdd(Body *);
        void FalseAdd(Body *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class CaseStm : public Stm {
      public:
        Exp *object;
        std::vector<Situation *> situations;
        CaseStm(Exp *_object) : Stm(ND_CASE_STM), object(_object) {}
        void SituaAdd(Situation *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
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
        void LoopAdd(Body *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class WhileStm : public Stm {
      public:
        Exp *condition = nullptr;
        Body *loop     = nullptr;
        WhileStm(Exp *_condition) : Stm(ND_WHILE_STM), condition(_condition) { _condition->father = this; }
        void LoopAdd(Body *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class StatementRepeat : public Stm {
      public:
        Exp *condition = nullptr;
        Body *loop     = nullptr;
        StatementRepeat() : Stm(ND_REPEAT_STM) {}
        void ConditionSetup(Exp *);
        void LoopAdd(Body *);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class GotoStm : public Stm {
      public:
        int label;
        GotoStm(int _label) : Stm(ND_GOTO_STM), label(_label) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class UnaryExp : public Exp {
      public:
        int opcode;
        Exp *operand;
        UnaryExp(int _opcode, Exp *_operand) : Exp(ND_UNARY_EXP), opcode(_opcode), operand(_operand) {
            _operand->father = this;
        }
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class BinaryExp : public Exp {
      public:
        int opcode;
        Exp *OPRFIRST, *OPRSECOND;
        BinaryExp(int _opcode, Exp *_OPRFIRST, Exp *_OPRSECOND)
            : Exp(ND_BINARY_EXP), opcode(_opcode), OPRFIRST(_OPRFIRST), OPRSECOND(_OPRSECOND) {
            _OPRFIRST->father = OPRSECOND->father = this;
        }
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class CallExp : public Exp {
      public:
        std::string name;
        std::vector<Exp *> args;
        CallExp(const std::string &_name) : Exp(ND_CALL_EXP), name(_name) {}
        void PARAMAdd(Exp *args);
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class EXPRESSIONConst : public Exp {
      public:
        Value *value;
        EXPRESSIONConst(Value *_value) : Exp(NX_CONST_EXP), value(_value) { returnVal = _value; }
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
    };

    class VariableExp : public Exp {
      public:
        std::string name;
        VariableExp(const std::string &_name) : Exp(ND_VARIABLE_EXP), name(_name) {}
        virtual llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context) override;
        bool SEMANT_CHECK_LEGAL() override;
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
        llvm::Value *PascalCodeCreate(ContextOfCodeCreate *context);
    };

    void visualizeTree(std::string filename, Base *root);
    Type *copyType(Type *src);
    bool isSameType(Type *type1, Type *type2);
    Base *findName(const std::string &name, Base *node);
    bool canFindLabel(const int &label, Base *node);
    ConstDef *findConst(const std::string &typeName, Base *node);
    Type *findType(const std::string &typeName, Base *node);
    Type *findVar(const std::string &typeName, Base *node);
    FuncDef *findFunction(const std::string &typeName, Base *node);

}  // namespace tree
#endif
