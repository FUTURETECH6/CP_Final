#include <iostream>
#include <vector>

#include "codegen.h"

/* Public Class Method */

/**
 * @brief find a Value* of a var by its name
 *
 * @param name
 * @return llvm::Value*
 */
llvm::Value *CodeGenContext::getValue(std::string name) {
    std::cout << "Start get value for `" << name << "`" << std::endl;

    llvm::Value *ret;
    if ((ret = this->pModule->getGlobalVariable(name)) != nullptr) {  // global
        std::cout << "[Success] Found global variable: " << name << std::endl;
        return ret;
    } else if ((ret = this->funcCur->getValueSymbolTable()->lookup(name)) != nullptr) {  // local
        std::cout << "[Success] Found local variable: " << name << std::endl;
        return ret;
    } else {  // Not found
        std::cout << "[Error] Undeclared variable: " << name << std::endl;
        exit(0);
    }
}

/**
 * @brief return a LLVM type by type in tree
 *
 * @param type
 * @return llvm::Type*
 */
llvm::Type *CodeGenContext::getLlvmType(tree::Type *type) {
    if (type == nullptr) {
        return llvm::Type::getVoidTy(globalContext);
    }

    switch (type->baseType) {
        case TY_INT: return llvm::Type::getInt32Ty(globalContext);
        case TY_REAL: return llvm::Type::getFloatTy(globalContext);
        case TY_CHAR: return llvm::Type::getInt8Ty(globalContext);
        case TY_BOOL: return llvm::Type::getInt1Ty(globalContext);

        case TY_ARRAY:
            return llvm::ArrayType::get(this->getLlvmType(type->childType[0]), type->indexEnd - type->indexStart + 1);

        case TY_RECORD: {
            std::vector<llvm::Type *> members;
            // StringRef Name
            auto const rcd = llvm::StructType::create(globalContext, type->name);

            for (auto &child : type->childType)
                members.push_back(this->getLlvmType(child));
            rcd->setBody(members);
            return rcd;
        }
        default: {
            auto curBlk = this->getCurCodeGenBlk();
            while (curBlk) {
                auto &typeMap = curBlk->typeMap;
                if (typeMap.find(type->name) != typeMap.end())
                    return this->getLlvmType(typeMap[type->name]);
                curBlk = curBlk->parent;
            }
            exit(0);
        }
    }
}

void CodeGenContext::generateCode(tree::Program &root, std::string file) {
    std::cout << "Generating code..." << std::endl;

    std::vector<llvm::Type *> argTypeVec;  // Arguments Type Vector for a func
    // void main, fix len of params
    llvm::FunctionType *mainType =
        llvm::FunctionType::get(llvm::Type::getVoidTy(globalContext),  // Type *Result: type of return value
            llvm::makeArrayRef(argTypeVec),                            // ArrayRef<Type *> Params: param list
            false);                                                    // bool isVarArg: whether sizable
    // 利用构造的main函数函数类型，构造一个main函数
    this->funcMain = llvm::Function::Create(mainType,  // FunctionType *Ty: 函数类型
        llvm::GlobalValue::InternalLinkage,            // LinkageTypes Linkage
        llvm::Twine("main"),                           // const Twine &N
        this->pModule);                                // Module *M
    // main entry
    llvm::BasicBlock *basBlock = llvm::BasicBlock::Create(globalContext,  // LLVMContext &Context
        llvm::Twine("entry"),                                             // const Twine &Name
        this->funcMain,                                                   // Function *Parent
        nullptr);                                                         // BasicBlock *InsertBefore

    this->printf = createPrintf();

    this->pushBlock(basBlock);  // push entry of program(main)
    this->funcCur = this->funcMain;
    root.codeGen(this);
    llvm::ReturnInst::Create(globalContext,  // LLVMContext &Context
        this->getCurBasicBlk());             // BasicBlock *InsertAtEnd
    this->popBlock();                        // 出栈

    std::cout << "Code is generated." << std::endl;

    // generate module pass
    llvm::legacy::PassManager passMgr;
    passMgr.add(llvm::createPrintModulePass(llvm::outs()));
    passMgr.run(*this->pModule);

    // generate .bc
    std::error_code errCode;
    llvm::raw_fd_ostream os(file, errCode, llvm::sys::fs::F_None);
    llvm::WriteBitcodeToFile(*this->pModule, os);
    os.flush();
}

void CodeGenContext::runCode() {
    std::cout << "Running code..." << std::endl;
    auto exeEngine = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(this->pModule)).create();
    exeEngine->finalizeObject();  // ensure the module is fully processed and is usable.
    llvm::ArrayRef<llvm::GenericValue> noArgs;
    exeEngine->runFunction(this->funcMain,  //  Function *f
        noArgs);                            //  ArrayRef<GenericValue>  ArgValues
    std::cout << "Code run." << std::endl;
}

int CodeGenContext::getRecordIndex(tree::Type *recType, std::string name) {
    for (int i = 0; i < recType->childType.size(); i++)
        if (name == recType->childType[i]->name)
            return i;

    std::cout << "[Error] Unknown name: `" << name << "` (in record: " << recType->name << ")" << std::endl;
    exit(0);
}

llvm::Value *CodeGenContext::getAryAddr(tree::BinaryExp *exp, CodeGenContext *context) {
    auto arrName = static_cast<tree::VariableExp *>(exp->operand1)->name;

    if (exp->operand1->nodeType == ND_VARIABLE_EXP) {  // 如果二元表达式第一个参数是变量表达式
        std::vector<llvm::Value *> arrIdx(2);
        arrIdx[0] = llvm::ConstantInt::get(globalContext, llvm::APInt(32, 0, true));  // 构建 llvm 整型常数，值为 0
        arrIdx[1] = exp->operand2->codeGen(context);  //  获取二元表达式的第二个变量的值

        return llvm::GetElementPtrInst::CreateInBounds(
            context->getValue(arrName),             // Value *Ptr                // 从 context 中找到数组变量
            llvm::ArrayRef<llvm::Value *>(arrIdx),  // ArrayRef<Value *> IdxList
            llvm::Twine("tempname"),                // const Twine &NameStr
            context->getCurBasicBlk());             // BasicBlock *InsertAtEnd
    } else {
        std::cout << "[Error] Wrong array type: " << arrName << std::endl;
        exit(0);
    }
}

/* Private Class Method */

llvm::Function *CodeGenContext::createPrintf() {
    std::vector<llvm::Type *> argTypes;
    argTypes.push_back(llvm::Type::getInt8PtrTy(globalContext));  // push 第一个参数的类型 char *

    llvm::FunctionType *printType = llvm::FunctionType::get(  // 构造函数类型
        llvm::Type::getInt32Ty(globalContext),                // Type *Result 			// 返回 int
        llvm::makeArrayRef(argTypes),                         // ArrayRef<Type *> Params  // 参数表
        true);                                                // bool isVarArg            // 是否为变长参数
    llvm::Function *funcPrintf    = llvm::Function::Create(   // 根据函数类型构造函数体
        printType,                                         // FunctionType *Ty      // 函数类型
        llvm::Function::ExternalLinkage,                   // LinkageTypes Linkage  // 定义外部链接
        llvm::Twine("printf"),                             // const Twine &N        // 函数名称
        this->pModule);                                    // Module *M             // 装载的 module
    funcPrintf->setCallingConv(llvm::CallingConv::C);         // 设置调用常数
    return funcPrintf;
}

/* Code Generation of tree node */

llvm::Value *tree::Body::codeGen(CodeGenContext *context) {
    for (auto &stm : this->stms) {
        stm->codeGen(context);
        std::cout << "[Success] A statment generated" << std::endl;
    }
    return nullptr;
}

llvm::Value *tree::Program::codeGen(CodeGenContext *context) {
    llvm::Value *lastDef = nullptr;

    if (this->define != nullptr) {
        for (tree::ConstDef *&constDef : this->define->constDef) {
            std::cout << "[Success] Program defined a const: " << constDef->name << std::endl;
            lastDef = constDef->codeGen(context);
        }
        for (tree::VarDef *&varDef : this->define->varDef) {
            std::cout << "[Success] Program defined a gloable variable: " << varDef->name << std::endl;
            varDef->isGlobal = true;
            lastDef          = varDef->codeGen(context);
        }
        for (tree::FuncDef *&funcDef : this->define->funcDef) {
            std::cout << "[Success] Program defined a function: " << funcDef->name << std::endl;
            lastDef = funcDef->codeGen(context);
        }
    }

    std::cout << "[Success] Program generated" << std::endl;
    this->body->codeGen(context);

    return lastDef;
}

llvm::Value *tree::Define::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::Situation::codeGen(CodeGenContext *context) {
    return this->solution->codeGen(context);
}

llvm::Value *tree::LabelDef::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::ConstDef::codeGen(CodeGenContext *context) {
    std::cout << "Const defining: " << this->name << std::endl;
    if (this->value->nodeType == NX_CONST_EXP) {                              // 如果值是常量
        tree::ConstantExp *opLeft = static_cast<ConstantExp *>(this->value);  //
        llvm::Value *alloc        = new llvm::AllocaInst(                     // 为常量分配空间
            context->getLlvmType(opLeft->returnType),                  // Type *Ty                 // 类型
            0,                                                         // unsigned AddrSpace
            llvm::Twine(this->name),               // const Twine &Name        // 常量名称
            context->getCurBasicBlk());            // BasicBlock *InsertAtEnd
        llvm::Value *store        = new llvm::StoreInst(  // 将常量的值赋到空间中
            opLeft->codeGen(context),              // Value *Val              // 常量的值
            alloc,                                 // Value *Ptr              // 常量的地址
            false,                                 // bool isVolatile         // 不可更改
            context->getCurBasicBlk());            // BasicBlock InsertAtEnd

        context->addConst(this->name, this->value);  // 添加常量到上下文中
        std::cout << "[Success] Const defined: " << this->name << std::endl;
        return store;
    } else {
        std::cout << "[Error] Wrong left value(not a const): " << this->name << std::endl;
        exit(0);
    }
}

llvm::Value *tree::TypeDef::codeGen(CodeGenContext *context) {
    context->getCurCodeGenBlk()->typeMap[this->name] = this->type;  // 定义类型
    return nullptr;
}

llvm::Value *tree::VarDef::codeGen(CodeGenContext *context) {
    std::cout << "Variable defining: " << this->name << std::endl;

    llvm::Value *alloc;
    if (this->isGlobal) {  // 如果是全局变量
        std::cout << ">> Global variable" << std::endl;

        if (this->type->baseType == TY_ARRAY) {  // 如果是数组
            std::cout << ">>>> Array variable" << std::endl;

            auto vec = std::vector<llvm::Constant *>();
            llvm::Constant *eleOfArr;
            switch (this->type->childType[0]->baseType) {  // 对全局变量进行初始化
                case TY_INT: eleOfArr = llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true); break;
                case TY_REAL: eleOfArr = llvm::ConstantFP ::get(llvm::Type::getFloatTy(globalContext), 0); break;
                case TY_CHAR: eleOfArr = llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), 0, true); break;
                case TY_BOOL: eleOfArr = llvm::ConstantInt::get(llvm::Type::getInt1Ty(globalContext), 0, true); break;
                default: std::cout << "[Warning] Uncomplete feature for global array of record" << std::endl; exit(0);
            }

            for (int i = 0; i < this->type->indexEnd - this->type->indexStart + 1; i++) {
                vec.push_back(eleOfArr);
            }

            llvm::ArrayType *arrType = static_cast<llvm::ArrayType *>(context->getLlvmType(this->type));
            llvm::Constant *arrConst = llvm::ConstantArray::get(arrType, vec);
            alloc                    = new llvm::GlobalVariable(*context->pModule,  // Module &M
                context->getLlvmType(this->type),  // Type *Ty               // 全局变量的类型
                false,                             // bool isConstant        // 是否常量
                llvm::GlobalValue::ExternalLinkage,  // LinkageTypes Linkage
                arrConst,                  // Constant *Initializer  // 全局变量初始化为 0
                llvm::Twine(this->name));  // const Twine &Name      // 变量名称
        } else if (this->type->baseType == TY_RECORD) {
            std::cout << "[Warning] Uncomplete feature for gloable record" << std::endl;
            exit(0);
        } else {  // Not Array
            llvm::Constant *arrConst;
            switch (this->type->baseType) {
                case TY_INT: arrConst = llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true); break;
                case TY_REAL: arrConst = llvm::ConstantFP ::get(llvm::Type::getFloatTy(globalContext), 0); break;
                case TY_BOOL: arrConst = llvm::ConstantInt::get(llvm::Type::getInt1Ty(globalContext), 0, true); break;
                case TY_CHAR: arrConst = llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), 0, true); break;
                default:;
            }

            alloc = new llvm::GlobalVariable(*context->pModule, context->getLlvmType(this->type), false,
                llvm::GlobalValue::ExternalLinkage, arrConst, llvm::Twine(this->name));
        }
    } else {                                   // Local Var
        alloc = new llvm::AllocaInst(          // 为局部变量分配地址
            context->getLlvmType(this->type),  // 类型
            0,
            llvm::Twine(this->name),  // 常量名称
            context->getCurBasicBlk());
    }
    std::cout << "[Success] Variable defined: " << this->name << std::endl;
    return alloc;
}

llvm::Value *tree::FuncDef::codeGen(CodeGenContext *context) {
    std::cout << "Function defining: " << this->name << std::endl;
    context->funcDefMap[this->name] = this;

    std::vector<llvm::Type *> argTy;
    for (int i = 0; i < this->argTypeVec.size(); i++)
        if (this->argFormalVec[i])  // 参数传递 引用/值
            argTy.push_back(llvm::Type::getInt32PtrTy(globalContext));
        else
            argTy.push_back(context->getLlvmType(this->argTypeVec[i]));

    auto funcType =
        llvm::FunctionType::get(context->getLlvmType(this->retType),  // Type *Ty                 // 函数返回值类型
            llvm::makeArrayRef(argTy),                                // ArrayRef<Type *> Params  // 参数类型
            false);                                                   // bool isVarArg            // 定长
    auto func = llvm::Function::Create(funcType, llvm::GlobalValue::InternalLinkage,
        llvm::Twine(this->name),  // 函数名
        context->pModule);

    auto block = llvm::BasicBlock::Create(globalContext, llvm::Twine("entry"), func,
        nullptr);  // BasicBlock *InsertBefore

    auto oldFunc = context->getCurFunc();
    context->setCurFunc(func);
    auto oldBlock            = context->getCurBasicBlk();
    context->parentMap[func] = oldFunc;

    context->pushBlock(block);

    llvm::Value *pArgValue;
    llvm::Argument *argItor = func->arg_begin();
    for (int i = 0; i < argTy.size(); i++) {
        llvm::Type *ty;
        if (this->argFormalVec[i]) {
            ty = llvm::Type::getInt32PtrTy(globalContext);
            std::cout << ">> Formal argument define: " << this->argNameVec[i] << std::endl;
        } else {
            ty = context->getLlvmType(this->argTypeVec[i]);
            std::cout << ">> Argument define: " << this->argNameVec[i] << std::endl;
        }
        llvm::Value *alloc = new llvm::AllocaInst(  // 为参数分配空间
            ty,                                     // 参数类型
            0,
            llvm::Twine(this->argNameVec[i]),  // 参数名
            context->getCurBasicBlk());
        pArgValue          = argItor++;
        pArgValue->setName(llvm::Twine(this->argNameVec[i]));
        new llvm::StoreInst(  // 存参数值
            pArgValue,        // 参数值
            alloc,            // 参数地址
            false,            // 非常量
            block);
    }

    if (this->retType) {
        new llvm::AllocaInst(  // 为返回值分配地址
            context->getLlvmType(this->retType), 0, llvm::Twine(this->name), context->getCurBasicBlk());
        std::cout << ">> Function return value declaration" << std::endl;
    }

    std::cout << ">> [Success] Function header part generated success!" << std::endl;

    if (this->define) {
        std::cout << ">> Function variable define" << std::endl;
        for (auto &i : this->define->varDef)
            i->codeGen(context);

        for (auto &i : this->define->funcDef)
            i->codeGen(context);

        std::cout << ">> [Success] Function define part generated success!" << std::endl;
    }

    this->body->codeGen(context);

    if (this->retType != nullptr) {
        std::cout << ">> Generating return value for function" << std::endl;
        llvm::Value *load = new llvm::LoadInst(  // 加载返回值的地址
            context->getValue(this->name), llvm::Twine(""), false, context->getCurBasicBlk());
        llvm::ReturnInst::Create(globalContext, load, context->getCurBasicBlk());
        std::cout << ">> Function returned" << std::endl;
    } else {
        std::cout << ">> Generating return void for procedure" << std::endl;
        llvm::ReturnInst::Create(globalContext, context->getCurBasicBlk());
        std::cout << ">> Procedure returned" << std::endl;
    }

    while (context->getCurBasicBlk() != oldBlock) {  // 函数定义完成
        context->popBlock();
    }
    context->setCurFunc(oldFunc);

    std::cout << "[Success] Defined function: " << this->name << std::endl;
    return func;
}

llvm::Value *tree::AssignStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating assignment statment..." << std::endl;

    if (this->leftVal->nodeType == ND_BINARY_EXP) {  // 如果左值为二元表达式
        tree::BinaryExp *op1 = static_cast<tree::BinaryExp *>(this->leftVal);
        if (op1->opcode == OP_INDEX) {                                           // 为取数组下标
            llvm::Value *elementPtr = CodeGenContext::getAryAddr(op1, context);  // 取得目标地址
            std::cout << "[Success] Assignment statment generate" << std::endl;
            return new llvm::StoreInst(this->rightVal->codeGen(context),  // 值
                elementPtr,                                               // 地址
                false, context->getCurBasicBlk());
        } else if (op1->opcode == OP_DOT) {
            // TODO
        } else {
            std::cout << "[Error] Wrong left value type" << std::endl;
            exit(0);
        }
    } else if (this->leftVal->nodeType == ND_VARIABLE_EXP) {  // 如果左值为变量
        tree::VariableExp *op1 = static_cast<tree::VariableExp *>(this->leftVal);
        if (op1->codeGen(context)->getType()->isArrayTy()) {  // 如果左值是数组类型
            std::cout << "[Error] Wrong left value type" << std::endl;
            exit(0);
        } else {  // 如果左值不为数组类型
            llvm::Value *tmp = context->getValue(op1->name);
            llvm::Value *load;
            do {
                load = tmp;
                tmp  = new llvm::LoadInst(tmp, llvm::Twine(""), false, context->getCurBasicBlk());
            } while (tmp->getType()->isPointerTy());

            return new llvm::StoreInst(this->rightVal->codeGen(context),  // 值（右）
                load,                                                     // 地址（左）
                false, context->getCurBasicBlk());
        }
    } else {  // 如果左值不为变量/二元表达式
        std::cout << "[Error] Wrong left value type" << std::endl;
        exit(0);
    }
    return nullptr;
}

llvm::Value *tree::CallStm::codeGen(CodeGenContext *context) {
    std::cout << "Calling function: " << this->name << std::endl;
    if (this->name == "write" || this->name == "writeln") {  // 对 write 进行特判
        bool isWriteln = (this->name == "writeln");
        std::string printfFormat;
        std::vector<llvm::Value *> printfArgs;

        for (tree::Exp *arg : this->args) {
            llvm::Value *argValue = arg->codeGen(context);  // 得到参数的值
            if (argValue->getType() == llvm::Type::getInt32Ty(globalContext)) {
                printfFormat += "%d";
                std::cout << ">> System call write variable previous name" << argValue->getName().str() << std::endl;
                printfArgs.push_back(argValue);
            } else if (argValue->getType()->isFloatTy()) {
                printfFormat += "%f";
                printfArgs.push_back(argValue);
            } else if (argValue->getType() == llvm::Type::getInt8PtrTy(globalContext)) {
                std::cout << "[Warning] string print is not supported" << std::endl;
                exit(0);
            } else {
                std::cout << "[Error] Unknown type for printf" << std::endl;
                exit(0);
            }
        }

        if (isWriteln) {
            printfFormat += "\n";
        }

        std::cout << "print format: " << printfFormat << std::endl;
        llvm::Constant *printfFormat_const    = llvm::ConstantDataArray::getString(  // 创建常量 char*
            globalContext, printfFormat.c_str());
        llvm::GlobalVariable *formatStringVar = new llvm::GlobalVariable(  // 创建一个全局变量
            *context->pModule,
            llvm::ArrayType::get(  // 变量类型 char*
                llvm::IntegerType::getInt8Ty(globalContext), strlen(printfFormat.c_str()) + 1),
            true,  // 常量
            llvm::GlobalValue::PrivateLinkage,
            printfFormat_const,  // 变量的值
            llvm::Twine(".str"));
        llvm::Constant *zero                  = llvm::Constant::getNullValue(  // 创建一个 0
            llvm::IntegerType::getInt32Ty(globalContext));
        std::vector<llvm::Constant *> arrIdx;
        arrIdx.push_back(zero);
        arrIdx.push_back(zero);
        llvm::Constant *var_ref = llvm::ConstantExpr::getGetElementPtr(
            llvm::ArrayType::get(llvm::IntegerType::getInt8Ty(globalContext), strlen(printfFormat.c_str()) + 1),
            formatStringVar,  // 全局变量
            arrIdx);          // 从 0 开始
        printfArgs.insert(printfArgs.begin(), var_ref);
        std::cout << "[Success] Print call generated" << std::endl;
        return llvm::CallInst::Create(
            context->printf, llvm::makeArrayRef(printfArgs), llvm::Twine(""), context->getCurBasicBlk());
    }

    // 非 write/writeln
    llvm::Function *func = context->pModule->getFunction(this->name.c_str());  // 找到这个函数
    if (func == nullptr) {
        std::cout << "[Error] Function not defined" << std::endl;
        exit(0);
    }
    std::vector<llvm::Value *> argValues;
    auto funcArgs_iter = func->arg_begin();
    for (tree::Exp *arg : this->args) {
        llvm::Value *funcArgValue = static_cast<llvm::Value *>(funcArgs_iter++);
        if (funcArgValue->getType()->isPointerTy()) {  // 如果这个参数是指针（全局变量）
            if (arg->nodeType == ND_VARIABLE_EXP) {    // 如果这个参数是变量
                llvm::Value *ptr = context->getValue(static_cast<tree::VariableExp *>(arg)->name);  // 取得变量的值
                while (ptr->getType() != llvm::Type::getInt32PtrTy(globalContext)) {
                    ptr = new llvm::LoadInst(ptr, llvm::Twine(""), false, context->getCurBasicBlk());
                }
                argValues.push_back(ptr);
            } else if (arg->nodeType == ND_BINARY_EXP) {
                BinaryExp *node = static_cast<BinaryExp *>(arg);
                if (node->opcode == OP_DOT) {  // 记录类型
                    if (node->operand2->nodeType == ND_VARIABLE_EXP) {
                        VariableExp *op2 = static_cast<VariableExp *>(node->operand2);
                        int index        = CodeGenContext::getRecordIndex(node->operand1->returnType, op2->name);
                        std::vector<llvm::Value *> arrIdx(2);
                        arrIdx[0] = llvm::ConstantInt::get(  // 0
                            globalContext, llvm::APInt(32, 0, true));
                        arrIdx[1] = llvm::ConstantInt::get(  // index
                            globalContext, llvm::APInt(32, index, true));
                        llvm::Value *ptr =
                            llvm::GetElementPtrInst::Create(context->getLlvmType(node->operand1->returnType),  // 类型
                                context->getValue(op2->name),                                                  // 值
                                arrIdx,           // 记录所存在的序号
                                llvm::Twine(""),  // 名称
                                context->getCurBasicBlk());
                        argValues.push_back(ptr);
                    } else {
                        std::cout << "[Error] operand2 of dot operation is not a variable exp type" << std::endl;
                        exit(0);
                    }
                } else if (node->opcode == OP_INDEX) {  // 数组类型
                    if (node->operand1->nodeType == ND_VARIABLE_EXP) {
                        VariableExp *op1 = static_cast<VariableExp *>(node->operand1);
                        std::vector<llvm::Value *> arrIdx(2);
                        arrIdx[0]        = llvm::ConstantInt::get(globalContext, llvm::APInt(32, 0, true));
                        arrIdx[1]        = node->operand2->codeGen(context);
                        llvm::Value *ptr = llvm::GetElementPtrInst::CreateInBounds(context->getValue(op1->name),
                            llvm::ArrayRef<llvm::Value *>(arrIdx), llvm::Twine("tempname"), context->getCurBasicBlk());
                        argValues.push_back(ptr);
                    } else {
                        std::cout << "[Error] Array's Ref is not an array type variable" << std::endl;
                        exit(0);
                    }
                }
            } else {
                std::cout << "[Error] Wrong left value type" << std::endl;
            }
        } else {
            argValues.push_back(arg->codeGen(context));
        }
    }

    std::cout << "[Success] Function called." << std::endl;
    return llvm::CallInst::Create(func, llvm::makeArrayRef(argValues), llvm::Twine(""), context->getCurBasicBlk());
}

llvm::Value *tree::LabelStm::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::IfStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating if statment" << std::endl;
    llvm::Value *cond = condition->codeGen(context);
    std::cout << ">> [Success] Condition generated" << std::endl;
    llvm::BasicBlock *trueBlock  = llvm::BasicBlock::Create(globalContext, llvm::Twine("then"), context->getCurFunc());
    llvm::BasicBlock *falseBlock = llvm::BasicBlock::Create(globalContext, llvm::Twine("else"), context->getCurFunc());
    llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(globalContext, llvm::Twine("merge"), context->getCurFunc());

    llvm::Value *ret = llvm::BranchInst::Create(trueBlock,  // BasicBlock *True
        falseBlock,                                         // BasicBlock *False
        cond,                                               // Value *cond
        context->getCurBasicBlk());                         // BasicBlock *InsertAtEnd

    context->pushBlock(trueBlock);
    this->trueBody->codeGen(context);
    std::cout << ">> [Success] True block generated" << std::endl;
    llvm::BranchInst::Create(  // 为真的语句生成完成，回到 merge
        mergeBlock, context->getCurBasicBlk());
    context->popBlock();
    context->pushBlock(falseBlock);
    if (this->falseBody != nullptr) {
        this->falseBody->codeGen(context);
        std::cout << ">> [Success] False block generated" << std::endl;
    }
    llvm::BranchInst::Create(  // 为假的语句生成完成，回到 merge
        mergeBlock, context->getCurBasicBlk());
    context->popBlock();
    context->pushBlock(mergeBlock);
    std::cout << "[Success] If statment generated" << std::endl;
    return ret;
}

llvm::Value *tree::CaseStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating case statment" << std::endl;
    llvm::BasicBlock *exitBlock = llvm::BasicBlock::Create(globalContext, llvm::Twine("exit"), context->getCurFunc());
    std::vector<llvm::BasicBlock *> blocks;
    std::cout << ">> Contains :" << situations.size() << "cases" << std::endl;

    llvm::Value *ret;
    for (int i = 0; i < this->situations.size(); i++) {
        for (int j = 0; j < this->situations[i]->caseVec.size(); j++) {
            llvm::BasicBlock *basBlock =
                llvm::BasicBlock::Create(globalContext, llvm::Twine("case"), context->getCurFunc());

            std::cout << ">> In case " << i << ":" << j << std::endl;
            tree::BinaryExp *cond = new tree::BinaryExp(OP_EQUAL, this->object, this->situations[i]->caseVec[j]);

            if (i == this->situations.size() - 1 && j == this->situations[i]->caseVec.size() - 1) {  // 最后一块
                ret = llvm::BranchInst::Create(  // 最后一块连接到 exitBlock
                    basBlock,                    // BasicBlock *IfTrue
                    exitBlock,                   // BasicBlock *IfFalse      // 不符合条件，退出
                    cond->codeGen(context),      // Value Cond
                    context->getCurBasicBlk());  // BasicBlock *InsertAtEnd
            } else {
                llvm::BasicBlock *nextBlock =
                    llvm::BasicBlock::Create(globalContext, llvm::Twine("next"), context->getCurFunc());
                llvm::BranchInst::Create(basBlock,  // BasicBlock *IfTrue       // 符合条件，这个
                    nextBlock,                      // BasicBlock *IfFalse      // 不符合条件，下一个
                    cond->codeGen(context),         // Value Cond
                    context->getCurBasicBlk());     // BasicBlock *InsertAtEnd
                context->pushBlock(nextBlock);
            }
        }
    }

    for (int i = 0; i < this->situations.size(); i++) {                  // 对于每一种 situation
        for (int j = 0; j < this->situations[i]->caseVec.size(); j++) {  // 每一种 situation 中的每一个 match
            llvm::BasicBlock *basBlock =
                llvm::BasicBlock::Create(globalContext, llvm::Twine("caseStmt"), context->getCurFunc());
            blocks.push_back(basBlock);  // 建立一个 block 并 push
        }
    }

    // llvm::Value *ret;
    for (int i = 0, p = 0; i < this->situations.size(); i++, p++) {
        for (int j = 0; j < this->situations[i]->caseVec.size(); j++, p++) {
            std::cout << "in case No." << i << std::endl;
            std::cout << "|__case's No." << j << std::endl;

            tree::BinaryExp *cond = new tree::BinaryExp(OP_EQUAL, this->object, this->situations[i]->caseVec[j]);

            llvm::BasicBlock *nextBlock;
            if (p == blocks.size() - 1) {
                ret = llvm::BranchInst::Create(blocks[p],  // BasicBlock *IfTrue
                    exitBlock,                             // BasicBlock *IfFalse
                    cond->codeGen(context),                // Value Cond
                    context->getCurBasicBlk());            // BasicBlock *InsertAtEnd
            } else {
                nextBlock = llvm::BasicBlock::Create(globalContext, "next", context->getCurFunc());

                llvm::BranchInst::Create(blocks[p],  // BasicBlock *IfTrue
                    nextBlock,                       // BasicBlock *IfFalse
                    cond->codeGen(context),          // Value Cond
                    context->getCurBasicBlk());      // BasicBlock *InsertAtEnd

                context->pushBlock(nextBlock);
            }
        }
    }

    for (int i = 0, p = 0; i < this->situations.size(); i++, p++) {
        for (int j = 0; j < this->situations[i]->caseVec.size(); j++, p++) {
            context->pushBlock(blocks[p]);
            this->situations[i]->codeGen(context);
            llvm::BranchInst::Create(exitBlock, context->getCurBasicBlk());
            std::cout << ">> [Success] In case " << i << ":" << j << std::endl;
            context->popBlock();
        }
    }

    context->pushBlock(exitBlock);

    return ret;
}

llvm::Value *tree::ForStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating for statement" << std::endl;
    llvm::BasicBlock *startBlock = llvm::BasicBlock::Create(globalContext, llvm::Twine("start"), context->getCurFunc());
    llvm::BasicBlock *loopBlock  = llvm::BasicBlock::Create(globalContext, llvm::Twine("loop"), context->getCurFunc());
    llvm::BasicBlock *exitBlock  = llvm::BasicBlock::Create(globalContext, llvm::Twine("exit"), context->getCurFunc());
    // 定义循环变量
    tree::VariableExp *loopVar   = new tree::VariableExp(this->iter);
    loopVar->returnType          = tree::findVar(this->iter, this);
    loopVar->name                = this->iter;
    tree::AssignStm *initLoopVar = new tree::AssignStm(  // 为循环变量赋初值
        loopVar, this->start);
    initLoopVar->codeGen(context);  // 生成赋初值的代码 (i := start)
    llvm::BranchInst::Create(       // 跳转入 startBlock
        startBlock, context->getCurBasicBlk());
    // 循环过程
    context->pushBlock(startBlock);                // startBlock
    tree::BinaryExp *cmp   = new tree::BinaryExp(  // 判断循环变量是否到达终点值
        OP_EQUAL, loopVar, this->end);
    llvm::Instruction *ret = llvm::BranchInst::Create(exitBlock,  // 到达终点值，跳入 exitBlock
        loopBlock,                                                // 未到达终点值，跳入 loopBlock
        cmp->codeGen(context), context->getCurBasicBlk());
    context->popBlock();
    context->pushBlock(loopBlock);  // loopBlock
    this->loop->codeGen(context);   // 生成代码块
    // 循环变量的更新
    tree::BinaryExp *update;
    tree::Value *tmp               = new tree::Value();
    tmp->baseType                  = TY_INT;
    tmp->val.intVal                = this->step;  // 1 为 to ， -1 为 downto
    tree::ConstantExp *int1        = new tree::ConstantExp(tmp);
    update                         = new tree::BinaryExp(  // i + 1 或 i - 1
        OP_ADD, loopVar, int1);
    tree::AssignStm *updateLoopVar = new tree::AssignStm(  // 更新循环变量的语句
        loopVar, update);
    updateLoopVar->codeGen(context);  // i := i + 1 或 i := i - 1
    llvm::BranchInst::Create(         // 跳入 startBlock
        startBlock, context->getCurBasicBlk());
    context->popBlock();
    context->pushBlock(exitBlock);  // exitBlock
    this->loop->codeGen(context);   // 最后一次循环
    std::cout << "[Success] For loop generated" << std::endl;
    return ret;
}

llvm::Value *tree::WhileStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating while statement" << std::endl;
    llvm::BasicBlock *startBlock = llvm::BasicBlock::Create(globalContext, llvm::Twine("start"), context->getCurFunc());
    llvm::BasicBlock *loopBlock  = llvm::BasicBlock::Create(globalContext, llvm::Twine("loop"), context->getCurFunc());
    llvm::BasicBlock *exitBlock  = llvm::BasicBlock::Create(globalContext, llvm::Twine("exit"), context->getCurFunc());
    llvm::BranchInst::Create(  // 首先跳转入 startBlock
        startBlock, context->getCurBasicBlk());
    context->pushBlock(startBlock);                         // startBlock
    llvm::Value *ret = llvm::BranchInst::Create(loopBlock,  // 符合 cond ，继续 loopBlock
        exitBlock,                                          // 不符合 cond ，跳入 exitBlock
        this->condition->codeGen(context), context->getCurBasicBlk());
    context->popBlock();
    context->pushBlock(loopBlock);  // loopBlock
    this->loop->codeGen(context);   // 生成代码块
    llvm::BranchInst::Create(       // 继续循环，跳入 startBlock
        startBlock, context->getCurBasicBlk());
    context->popBlock();
    context->pushBlock(exitBlock);  // exitBlock
    std::cout << "[Success] While loop generated" << std::endl;
    return ret;
}

llvm::Value *tree::RepeatStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating repeat statement" << std::endl;
    llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(globalContext, llvm::Twine("loop"), context->getCurFunc());
    llvm::BasicBlock *exitBlock = llvm::BasicBlock::Create(globalContext, llvm::Twine("exit"), context->getCurFunc());
    llvm::BranchInst::Create(  // 跳入 loopBlock
        loopBlock, context->getCurBasicBlk());
    context->pushBlock(loopBlock);                                // loopBlock
    this->loop->codeGen(context);                                 // 生成代码块
    llvm::Value *cond      = this->condition->codeGen(context);   // 跳转条件
    llvm::Instruction *ret = llvm::BranchInst::Create(exitBlock,  // 符合 until 条件，退出
        loopBlock,                                                // 不符合，跳回 loopBlock
        cond, context->getCurBasicBlk());
    context->popBlock();
    context->pushBlock(exitBlock);  // exitBlock
    std::cout << "[Success] Repeat loop generated" << std::endl;
    return ret;
}

llvm::Value *tree::GotoStm::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::UnaryExp::codeGen(CodeGenContext *context) {
    switch (this->opcode) {
        case OP_NOT:
            return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT,
                this->operand->codeGen(context), llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true),
                llvm::Twine(""), context->getCurBasicBlk());
        case OP_OPPO:
            if (this->operand->returnType->baseType == TY_INT) {  // -m = 0 - m
                return llvm::BinaryOperator::Create(llvm::Instruction::Sub,
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true),
                    this->operand->codeGen(context), llvm::Twine(""), context->getCurBasicBlk());
            } else if (this->operand->returnType->baseType == TY_REAL) {
                return llvm::BinaryOperator::Create(llvm::Instruction::FSub,
                    llvm::ConstantFP::get(globalContext, llvm::APFloat(0.)), this->operand->codeGen(context),
                    llvm::Twine(""), context->getCurBasicBlk());
            }
            break;
        case OP_ABS:
            if (this->operand->returnType->baseType == TY_INT) {
                llvm::Value *cond = llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT,
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true),
                    this->operand->codeGen(context), llvm::Twine(""), context->getCurBasicBlk());
                llvm::BasicBlock *negBlock =
                    llvm::BasicBlock::Create(globalContext, llvm::Twine("neg"), context->getCurFunc());
                llvm::BasicBlock *posBlock =
                    llvm::BasicBlock::Create(globalContext, llvm::Twine("pos"), context->getCurFunc());
                llvm::BasicBlock *mergeBlock =
                    llvm::BasicBlock::Create(globalContext, llvm::Twine("merge"), context->getCurFunc());
                llvm::Value *ret = llvm::BranchInst::Create(negBlock,  // < 0, negBlock
                    posBlock,                                          // > 0, posBlock
                    cond, context->getCurBasicBlk());
                context->pushBlock(negBlock);  // negBlock
                llvm::BinaryOperator::Create(  // abs(m) = -m
                    llvm::Instruction::Sub, llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true),
                    this->operand->codeGen(context), llvm::Twine(""), context->getCurBasicBlk());
                llvm::BranchInst::Create(  // 跳入 mergeBlock
                    mergeBlock, context->getCurBasicBlk());
                context->popBlock();
                context->pushBlock(posBlock);     // faseBlock
                this->operand->codeGen(context);  // abs(m) = m
                llvm::BranchInst::Create(         // 跳入 mergeBlock
                    mergeBlock, context->getCurBasicBlk());
                context->popBlock();
                context->pushBlock(mergeBlock);  // mergeBlock
                return ret;
            } else if (this->operand->returnType->baseType == TY_REAL) {
                llvm::Value *cond = llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT,
                    llvm::ConstantFP::get(globalContext, llvm::APFloat(0.)), this->operand->codeGen(context),
                    llvm::Twine(""), context->getCurBasicBlk());
                llvm::BasicBlock *negBlock =
                    llvm::BasicBlock::Create(globalContext, llvm::Twine("neg"), context->getCurFunc());
                llvm::BasicBlock *posBlock =
                    llvm::BasicBlock::Create(globalContext, llvm::Twine("pos"), context->getCurFunc());
                llvm::BasicBlock *mergeBlock =
                    llvm::BasicBlock::Create(globalContext, llvm::Twine("merge"), context->getCurFunc());
                llvm::Value *ret = llvm::BranchInst::Create(negBlock, posBlock, cond, context->getCurBasicBlk());
                context->pushBlock(negBlock);
                llvm::BinaryOperator::Create(llvm::Instruction::FSub,
                    llvm::ConstantFP::get(globalContext, llvm::APFloat(0.)), this->operand->codeGen(context),
                    llvm::Twine(""), context->getCurBasicBlk());
                llvm::BranchInst::Create(mergeBlock, context->getCurBasicBlk());
                context->popBlock();
                context->pushBlock(posBlock);
                this->operand->codeGen(context);
                llvm::BranchInst::Create(mergeBlock, context->getCurBasicBlk());
                context->popBlock();
                context->pushBlock(mergeBlock);
                return ret;
            }
        case OP_PRED:
            return llvm::BinaryOperator::Create(  // m - 1
                llvm::Instruction::Sub, this->operand->codeGen(context),
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), 1, true), llvm::Twine(""),
                context->getCurBasicBlk());
        case OP_SUCC:
            return llvm::BinaryOperator::Create(  // m + 1
                llvm::Instruction::Add, this->operand->codeGen(context),
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), 1, true), llvm::Twine(""),
                context->getCurBasicBlk());
        case OP_ODD:
            return llvm::BinaryOperator::Create(  // 0 & m
                llvm::Instruction::And, llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true),
                this->operand->codeGen(context), llvm::Twine(""), context->getCurBasicBlk());
        case OP_CHR:
            return llvm::CastInst::CreateIntegerCast(this->operand->codeGen(context),
                llvm::Type::getInt8Ty(globalContext), true, llvm::Twine(""), context->getCurBasicBlk());
        case OP_ORD:
            return llvm::CastInst::CreateIntegerCast(this->operand->codeGen(context),
                llvm::Type::getInt32Ty(globalContext), true, llvm::Twine(""), context->getCurBasicBlk());
    }
    return nullptr;
}

llvm::Value *tree::BinaryExp::codeGen(CodeGenContext *context) {
    if (this->opcode == OP_DOT) {                           // record
        if (this->operand2->nodeType == ND_VARIABLE_EXP) {  // 操作符 2 是变量
            tree::VariableExp *op2 = static_cast<tree::VariableExp *>(this->operand2);
            int index              = CodeGenContext::getRecordIndex(  // 找到所求变量在记录中的位置
                this->operand1->returnType, op2->name);
            std::vector<llvm::Value *> arrIdx(2);
            arrIdx[0]           = llvm::ConstantInt::get(  // 0
                globalContext, llvm::APInt(32, 0, true));
            arrIdx[1]           = llvm::ConstantInt::get(                // index
                globalContext, llvm::APInt(32, index, true));  // create member_index
            llvm::Value *memPtr = llvm::GetElementPtrInst::Create(context->getLlvmType(this->operand1->returnType),
                context->getValue(op2->name), arrIdx, llvm::Twine(""), context->getCurBasicBlk());
            return new llvm::LoadInst(memPtr, llvm::Twine(""), false, context->getCurBasicBlk());
        } else {
            std::cout << "[Error] Wrong member variable for record" << std::endl;
            exit(0);
        }
    } else if (this->opcode == OP_INDEX) {
        if (this->operand1->nodeType == ND_VARIABLE_EXP) {
            tree::VariableExp *op1 = static_cast<tree::VariableExp *>(this->operand1);
            std::vector<llvm::Value *> arrIdx(2);
            arrIdx[0]           = llvm::ConstantInt::get(globalContext, llvm::APInt(32, 0, true));
            arrIdx[1]           = this->operand2->codeGen(context);
            llvm::Value *memPtr = llvm::GetElementPtrInst::CreateInBounds(context->getValue(op1->name),
                llvm::ArrayRef<llvm::Value *>(arrIdx), llvm::Twine("tempname"), context->getCurBasicBlk());
            return new llvm::LoadInst(memPtr, llvm::Twine(""), false, context->getCurBasicBlk());
        } else {
            std::cout << "[Error] Array's Ref is not an array type variable" << std::endl;
            exit(0);
        }
    }
    llvm::Value *op1Val = this->operand1->codeGen(context);
    llvm::Value *op2Val = this->operand2->codeGen(context);

    if (op1Val->getType()->isFloatTy() || op2Val->getType()->isFloatTy()) {  // 如果是浮点数运算
        switch (this->opcode) {
            case OP_ADD:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::FAdd, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MINUS:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::FSub, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MULTI:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::FMul, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_RDIV:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::FDiv, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_DDIV:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::SDiv, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MOD:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::SRem, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_AND:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::And, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_OR:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Or, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            // logical
            case OP_SMALL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLT, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_LARGE:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_SMALL_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGE, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_LARGE_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLE, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_EQ, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_NOT_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_NE, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            default:
                std::cout << "[Error] Unknown type of opcode:" << opcode << std::endl;  // not know what to do
                exit(0);
        }
    } else {  // 整型运算
        switch (opcode) {
            case OP_ADD:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Add, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MINUS:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Sub, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MULTI:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Mul, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_RDIV:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::UDiv, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_DDIV:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::SDiv, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MOD:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::SRem, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_AND:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::And, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_OR:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Or, op1Val, op2Val, llvm::Twine(""), context->getCurBasicBlk());
            case OP_SMALL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLT, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_LARGE:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_SMALL_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGE, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_LARGE_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLE, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_EQ, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_NOT_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_NE, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBasicBlk());
            default: std::cout << "[Error] Unknown type of opcode:" << opcode << std::endl; exit(0);
        }
    }
}

llvm::Value *tree::CallExp::codeGen(CodeGenContext *context) {
    std::cout << "Creating calling " << std::endl;
    llvm::Function *func = context->pModule->getFunction(this->name.c_str());
    if (func == nullptr) {
        std::cout << "[Error] Function: " << name << " not defined" << std::endl;
        exit(0);
    }
    std::vector<llvm::Value *> argValues;
    auto funcArgs_iter = func->arg_begin();
    for (tree::Exp *arg : this->args) {
        llvm::Value *funcArgValue = static_cast<llvm::Value *>(funcArgs_iter++);
        if (funcArgValue->getType()->isPointerTy()) {  // 如果这个参数是指针（全局变量）
            if (arg->nodeType == ND_VARIABLE_EXP) {    // 如果这个参数是变量
                llvm::Value *ptr = context->getValue(static_cast<tree::VariableExp *>(arg)->name);  // 取得变量的值
                while (ptr->getType() != llvm::Type::getInt32PtrTy(globalContext)) {
                    ptr = new llvm::LoadInst(ptr, llvm::Twine(""), false, context->getCurBasicBlk());
                }
                argValues.push_back(ptr);
            } else if (arg->nodeType == ND_BINARY_EXP) {
                BinaryExp *node = static_cast<BinaryExp *>(arg);
                if (node->opcode == OP_DOT) {  // 记录类型
                    if (node->operand2->nodeType == ND_VARIABLE_EXP) {
                        VariableExp *op2 = static_cast<VariableExp *>(node->operand2);
                        int index        = CodeGenContext::getRecordIndex(node->operand1->returnType, op2->name);
                        std::vector<llvm::Value *> arrIdx(2);
                        arrIdx[0] = llvm::ConstantInt::get(  // 0
                            globalContext, llvm::APInt(32, 0, true));
                        arrIdx[1] = llvm::ConstantInt::get(  // index
                            globalContext, llvm::APInt(32, index, true));
                        llvm::Value *ptr =
                            llvm::GetElementPtrInst::Create(context->getLlvmType(node->operand1->returnType),  // 类型
                                context->getValue(op2->name),                                                  // 值
                                arrIdx,           // 记录所存在的序号
                                llvm::Twine(""),  // 名称
                                context->getCurBasicBlk());
                        argValues.push_back(ptr);
                    } else {
                        std::cout << "[Error] operand2 of dot operation is not a variable exp type" << std::endl;
                        exit(0);
                    }
                } else if (node->opcode == OP_INDEX) {  // 数组类型
                    if (node->operand1->nodeType == ND_VARIABLE_EXP) {
                        VariableExp *op1 = static_cast<VariableExp *>(node->operand1);
                        std::vector<llvm::Value *> arrIdx(2);
                        arrIdx[0]        = llvm::ConstantInt::get(globalContext, llvm::APInt(32, 0, true));
                        arrIdx[1]        = node->operand2->codeGen(context);
                        llvm::Value *ptr = llvm::GetElementPtrInst::CreateInBounds(context->getValue(op1->name),
                            llvm::ArrayRef<llvm::Value *>(arrIdx), llvm::Twine("tempname"), context->getCurBasicBlk());
                        argValues.push_back(ptr);
                    } else {
                        std::cout << "[Error] Array's Ref is not an array type variable" << std::endl;
                        exit(0);
                    }
                }
            } else {
                std::cout << "[Error] Wrong left value type" << std::endl;
            }
        } else {
            argValues.push_back(arg->codeGen(context));
        }
    }
    return llvm::CallInst::Create(func, llvm::makeArrayRef(argValues), llvm::Twine(""), context->getCurBasicBlk());
}

llvm::Value *tree::ConstantExp::codeGen(CodeGenContext *context) {
    return value->codeGen(context);
}

llvm::Value *tree::VariableExp::codeGen(CodeGenContext *context) {
    std::cout << "loading variable: `" << this->name << "`" << std::endl;
    llvm::Value *ptr = context->getValue(this->name);
    ptr              = new llvm::LoadInst(ptr, llvm::Twine(""), false, context->getCurBasicBlk());
    if (ptr->getType()->isPointerTy()) {
        ptr = new llvm::LoadInst(ptr, llvm::Twine(""), false, context->getCurBasicBlk());
    }
    return ptr;
}

llvm::Value *tree::Type::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::Value::codeGen(CodeGenContext *context) {
    switch (this->baseType) {
        case TY_INT: return llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), this->val.intVal, true);
        case TY_REAL: return llvm::ConstantFP::get(globalContext, llvm::APFloat(this->val.realVal));
        case TY_CHAR: return llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), this->val.charVal, true);
        case TY_BOOL: return llvm::ConstantInt::get(llvm::Type::getInt1Ty(globalContext), this->val.boolVal, true);
        default: return nullptr;
    }
}
