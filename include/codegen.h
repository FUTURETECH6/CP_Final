#ifndef CODEGEN_H
#define CODEGEN_H

#include <iostream>
#include <typeinfo>

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

  public:
    std::map<std::string, tree::Type *> typeMap;
};

class CodeGenContext {
  private:
    std::stack<CodeGenBlk *> CodeGenStack;
    llvm::Function *funcMain;
    llvm::Function *funcCur;

  public:
    llvm::Module *pModule = new llvm::Module("main", globalContext);
    std::map<llvm::Function *, llvm::Function *> parentMap;
    llvm::Function *printf;
    std::map<std::string, tree::FuncDef *> funcDefMap;

    void generateCode(tree::Program &root, std::string file = "a.bc");
    void runCode();
    llvm::Type *getLlvmType(tree::Type *type);
    llvm::Value *getValue(std::string name);

    llvm::Function *getCurFunc() { return this->funcCur; }
    void setCurFunc(llvm::Function *func) { this->funcCur = func; }
    llvm::BasicBlock *getCurBlk() { return CodeGenStack.top()->block; }
    CodeGenBlk *getCurCodeGenBlk() { return CodeGenStack.top(); }
    void pushBlock(llvm::BasicBlock *block) {
        std::cout << "pushing..." << std::endl;
        CodeGenStack.push(new CodeGenBlk());
        CodeGenStack.top()->block = block;
    }
    void popBlock() {
        std::cout << "poping..." << std::endl;
        CodeGenBlk *top = CodeGenStack.top();
        CodeGenStack.pop();
        delete top;
    }
    void insertConst(std::string name, tree::Exp *const_v) {
        CodeGenStack.top()->constMap[name] = const_v;
    }
};

#endif
