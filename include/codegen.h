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

static llvm::LLVMContext MyContext;

llvm::Function *createPrintf(CodeGenContext *context);

class CodeGenBlk {
  public:
    llvm::BasicBlock *block;
    CodeGenBlk *parent;
    std::map<std::string, tree::Exp *> constMap;
    std::map<std::string, tree::Type *> typeMap;
};

class CodeGenContext {
  private:
    std::stack<CodeGenBlk *> CodeGenStack;
    llvm::Function *funcMain;
    llvm::Function *funcCur;

  public:
    llvm::Module *module = new llvm::Module("main", MyContext);
    std::map<llvm::Function *, llvm::Function *> parentMap;
    llvm::Function *printf;
    std::map<std::string, tree::FunctionDef *> funcDefMap;

    llvm::Function *getCurFunc() { return this->funcCur; }

    void setCurFunc(llvm::Function *func) { this->funcCur = func; }

    llvm::Type *getLlvmType(tree::Type *type);

    void generateCode(tree::Program &root) {
        std::cout << "Generating code...\n";

        std::vector<llvm::Type *> argTypeVec;  // 对于一个函数，llvm需要一个参数类型表
        // 构造一个main函数的函数类型，返回值为void，定长参数
        llvm::FunctionType *funcType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(MyContext),  // Type *Result: 返回值类型
            llvm::makeArrayRef(argTypeVec),    // ArrayRef<Type *> Params: 参数列表
            false);                            // bool isVarArg: 是否为变长参数
        // 利用构造的main函数函数类型，构造一个main函数
        this->funcMain = llvm::Function::Create(funcType,  // FunctionType *Ty: 函数类型
            llvm::GlobalValue::InternalLinkage,            // LinkageTypes Linkage
            llvm::Twine("main"),                           // const Twine &N
            this->module);                                 // Module *M
        // 主程序入口（main）
        llvm::BasicBlock *basBlock =
            llvm::BasicBlock::Create(MyContext,  // LLVMContext &Context
                llvm::Twine("entry"),            // const Twine &Name
                this->funcMain,                  // Function *Parent
                nullptr);                        // BasicBlock *InsertBefore

        this->printf = createPrintf(this);

        this->pushBlock(basBlock);  // 将程序入口（main函数的block）压栈
        this->funcCur = this->funcMain;
        root.codeGen(this);
        llvm::ReturnInst::Create(MyContext,  // LLVMContext &Context
            this->getCurBlock());            // BasicBlock *InsertAtEnd
        this->popBlock();                    // 出栈

        std::cout << "Code is generated.\n";

        // 生成中间代码
        llvm::legacy::PassManager pm;
        pm.add(llvm::createPrintModulePass(llvm::outs()));
        pm.run(*this->module);

        // 二进制代码
        std::error_code EC;
        llvm::raw_fd_ostream OS("a.bc", EC, llvm::sys::fs::F_None);
        llvm::WriteBitcodeToFile(*this->module, OS);
        OS.flush();
    }

    llvm::GenericValue runCode() {
        std::cout << "Running code...\n";

        llvm::ExecutionEngine *exeEngine =
            llvm::EngineBuilder(std::unique_ptr<llvm::Module>(this->module)).create();
        exeEngine->finalizeObject();  // ensure the module is fully processed and is usable.
        llvm::ArrayRef<llvm::GenericValue> noArgs;
        llvm::GenericValue value = exeEngine->runFunction(this->funcMain,  //  Function *f
            noArgs);  //  ArrayRef<GenericValue>  ArgValues
        std::cout << "Code run.\n";
        return value;
    }

    llvm::BasicBlock *getCurBlock() { return CodeGenStack.top()->block; }

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

    llvm::Value *getValue(std::string name) {
        std::cout << "Start get value for " << name << std::endl;

        llvm::Value *ret;
        if ((ret = this->funcCur->getValueSymbolTable()->lookup(name)) !=
            nullptr) {  // 在 curFunc 的局部变量中找到
            std::cout << "[Success] Found local variable: " << name << std::endl;
            return ret;
        } else if ((ret = this->module->getGlobalVariable(name)) !=
                   nullptr) {  // 在 module 的全局变量中找到
            std::cout << "[Success] Found global variable: " << name << std::endl;
            return ret;
        } else {  // 找不到
            std::cout << "[Error] Undeclared variable: " << name << std::endl;
            exit(0);
        }
    }
};

#endif
