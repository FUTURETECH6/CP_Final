#ifndef CODEGEN_H
#define CODEGEN_H

#include <iostream>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitstreamReader.h>
#include <llvm/Bitcode/BitstreamWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include "tree.h"

static llvm::LLVMContext globalContext;

llvm::Function *createPrintf(CodeGenContext *context);

struct CodeGenBlk {
    friend class CodeGenContext;

  private:
    llvm::BasicBlock *block;
    CodeGenBlk *parent;
    std::map<std::string, tree::Exp *> constMap;

    CodeGenBlk(llvm::BasicBlock *_block) : block(_block) {}

  public:
    std::map<std::string, tree::Type *> typeMap;
};

class CodeGenContext {
  private:
    std::stack<CodeGenBlk *> CodeGenStack;
    llvm::Function *funcMain;
    llvm::Function *funcCur;

    llvm::Function *createPrintf();

  public:
    llvm::Module *pModule = new llvm::Module("main", globalContext);
    std::map<llvm::Function *, llvm::Function *> parentMap;
    llvm::Function *printf;
    std::map<std::string, tree::FuncDef *> funcDefMap;

    void generateCode(tree::Program &root, std::string file = "a.bc");
    void runCode();
    llvm::Type *getLlvmType(tree::Type *type);
    llvm::Value *getValue(std::string name);

    // For record, find its index
    static int getRecordIndex(tree::Type *recType, std::string name);
    // Get array[offset]'s addr
    static llvm::Value *getAryAddr(tree::BinaryExp *exp, CodeGenContext *context);

    inline llvm::Function *getCurFunc() { return this->funcCur; }
    inline void setCurFunc(llvm::Function *func) { this->funcCur = func; }
    inline llvm::BasicBlock *getCurBasicBlk() { return CodeGenStack.top()->block; }
    inline CodeGenBlk *getCurCodeGenBlk() { return CodeGenStack.top(); }
    inline void pushBlock(llvm::BasicBlock *block) { CodeGenStack.push(new CodeGenBlk(block)); }
    inline void popBlock() {
        auto top = CodeGenStack.top();
        CodeGenStack.pop();
        delete top;
    }
    inline void addConst(std::string name, tree::Exp *_const) { CodeGenStack.top()->constMap[name] = _const; }
};

#endif
