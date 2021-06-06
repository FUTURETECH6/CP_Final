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

llvm::Function *PRINTFBUILD(ContextOfCodeCreate *context);

struct CodeGenBlk {
    friend class ContextOfCodeCreate;

  private:
    llvm::BasicBlock *block;
    CodeGenBlk *parent;
    std::map<std::string, tree::Exp *> constMap;

    CodeGenBlk(llvm::BasicBlock *_block) : block(_block) {}

  public:
    std::map<std::string, tree::Type *> typeMap;
};

class ContextOfCodeCreate {
  private:
    std::stack<CodeGenBlk *> STACKofCodeRUN;
    llvm::Function *MAINOFFUNCTION;
    llvm::Function *CURRENCTFUNCION;

    llvm::Function *PRINTFBUILD();

  public:
    llvm::Module *pModule = new llvm::Module("main", globalContext);
    std::map<llvm::Function *, llvm::Function *> MAPOFPARENT;
    llvm::Function *printf;
    std::map<std::string, tree::FuncDef *> MAPOFDEFFUNC;

    void CODEGENER(tree::Program &root, std::string file = "a.bc");
    void runCode();
    llvm::Type *LLVMTYPERET(tree::Type *type);
    llvm::Value *VALUERETGET(std::string name);

    // For record, find its index
    static int INDEXOFRECRETURN(tree::Type *recType, std::string name);
    // Get array[offset]'s addr
    static llvm::Value *ARRAYADDRRETURN(tree::BinaryExp *exp, ContextOfCodeCreate *context);

    inline llvm::Function *CURRECENTFUNCRETURN() { return this->CURRENCTFUNCION; }
    inline void CURRECENTFUNCSET(llvm::Function *func) { this->CURRENCTFUNCION = func; }
    inline llvm::BasicBlock *BASEBLKcurRETURN() { return STACKofCodeRUN.top()->block; }
    inline CodeGenBlk *BASECODEBLKcurRETURN() { return STACKofCodeRUN.top(); }
    inline void BLKpush(llvm::BasicBlock *block) { STACKofCodeRUN.push(new CodeGenBlk(block)); }
    inline void BLKpop() {
        auto Stacktop = STACKofCodeRUN.top();
        STACKofCodeRUN.pop();
        delete Stacktop;
    }
    inline void CONSTPLUS(std::string name, tree::Exp *_const) { STACKofCodeRUN.top()->constMap[name] = _const; }
};

#endif
