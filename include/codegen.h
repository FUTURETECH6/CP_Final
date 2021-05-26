#ifndef CODEGEN_H
#define CODEGEN_H

#include <iostream>

#include "tree.h"
#include <set>
#include <stack>

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
#include <typeinfo>

static llvm::LLVMContext MyContext;

llvm::Function *createPrintf(CodeGenContext *context);

class CodeGenBlock {
  public:
    llvm::BasicBlock *block;
    CodeGenBlock *parent;
    std::map<std::string, tree::Exp *> const_locals;
    std::map<std::string, tree::Type *> typedefs;
};

class CodeGenContext {
  private:
    std::stack<CodeGenBlock *> blocks;
    llvm::Function *mainFunction;
    llvm::Function *currentFunction;

  public:
    //    llvm::LLVMContext myContext = new llvm;
    llvm::Module *module = new llvm::Module("main", MyContext);
    std::map<llvm::Function *, llvm::Function *> parent;
    llvm::Function *printf;
    std::map<std::string, tree::FunctionDef *> defined_functions;

    CodeGenContext() {}

    llvm::Function *getCurFunc() { return this->currentFunction; }

    void setCurFunc(llvm::Function *func) { this->currentFunction = func; }

    llvm::Type *getLLVMTy(tree::Type *type) {
        if (type == nullptr) {
            return llvm::Type::getVoidTy(MyContext);
        }

        switch (type->base_type) {
            case TY_INTEGER: return llvm::Type::getInt32Ty(MyContext);
            case TY_REAL: return llvm::Type::getFloatTy(MyContext);
            case TY_CHAR: return llvm::Type::getInt8Ty(MyContext);
            case TY_BOOLEAN: return llvm::Type::getInt1Ty(MyContext);

            case TY_ARRAY: {
                return llvm::ArrayType::get(this->getLLVMTy(type->child_type[0]),
                    type->array_end - type->array_start + 1);
            }

            case TY_RECORD: {
                std::vector<llvm::Type *> members;
                for (tree::Type *child : type->child_type) {
                    members.push_back(this->getLLVMTy(child));
                }
                llvm::StructType *const rcd = llvm::StructType::create(MyContext,
                    type->name);  // StringRef Name
                rcd->setBody(members);
                return rcd;
            }

            default: {
                CodeGenBlock *curBlock = this->getCurCodeGenBlock();
                while (curBlock != nullptr) {
                    std::map<std::string, tree::Type *> typeTable = curBlock->typedefs;
                    if (typeTable.find(type->name) != typeTable.end()) {
                        return this->getLLVMTy(typeTable[type->name]);
                    }
                    curBlock = curBlock->parent;
                }
                exit(0);
            }
        }
    }

    void generateCode(tree::Program &root) {
        std::cout << "Generating code...\n";

        std::vector<llvm::Type *> argTypes;  // 对于一个函数，llvm需要一个参数类型表
        // 构造一个main函数的函数类型，返回值为void，定长参数
        llvm::FunctionType *funcType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(MyContext),  // Type *Result: 返回值类型
            llvm::makeArrayRef(argTypes),      // ArrayRef<Type *> Params: 参数列表
            false);                            // bool isVarArg: 是否为变长参数
        // 利用构造的main函数函数类型，构造一个main函数
        this->mainFunction = llvm::Function::Create(funcType,  // FunctionType *Ty: 函数类型
            llvm::GlobalValue::InternalLinkage,                // LinkageTypes Linkage
            llvm::Twine("main"),                               // const Twine &N
            this->module);                                     // Module *M
        // 主程序入口（main）
        llvm::BasicBlock *basBlock =
            llvm::BasicBlock::Create(MyContext,  // LLVMContext &Context
                llvm::Twine("entry"),            // const Twine &Name
                this->mainFunction,              // Function *Parent
                nullptr);                        // BasicBlock *InsertBefore

        this->printf = createPrintf(this);

        this->pushBlock(basBlock);  // 将程序入口（main函数的block）压栈
        this->currentFunction = this->mainFunction;
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
        llvm::GenericValue value = exeEngine->runFunction(this->mainFunction,  //  Function *f
            noArgs);  //  ArrayRef<GenericValue>  ArgValues
        std::cout << "Code run.\n";
        return value;
    }

    llvm::BasicBlock *getCurBlock() { return blocks.top()->block; }

    CodeGenBlock *getCurCodeGenBlock() { return blocks.top(); }

    void pushBlock(llvm::BasicBlock *block) {
        std::cout << "pushing..." << std::endl;
        blocks.push(new CodeGenBlock());
        blocks.top()->block = block;
    }
    void popBlock() {
        std::cout << "poping..." << std::endl;
        CodeGenBlock *top = blocks.top();
        blocks.pop();
        delete top;
    }

    void insertConst(std::string name, tree::Exp *const_v) {
        blocks.top()->const_locals[name] = const_v;
    }

    llvm::Value *getValue(std::string name) {
        std::cout << "Start get value for " << name << std::endl;

        llvm::Value *ret;
        if ((ret = this->currentFunction->getValueSymbolTable()->lookup(name)) !=
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
