#include <iostream>
#include <vector>

#include "codegen.h"

//--------------------------------------------------------------

llvm::Function *createPrintf(CodeGenContext *context) {
    std::vector<llvm::Type *> argTypes;
    argTypes.push_back(llvm::Type::getInt8PtrTy(MyContext));  // push 第一个参数的类型 char *
    llvm::FunctionType *printType = llvm::FunctionType::get(  // 构造函数类型
        llvm::Type::getInt32Ty(MyContext),                    // Type *Result 			// 返回 int
        llvm::makeArrayRef(argTypes),  // ArrayRef<Type *> Params  // 参数表
        true);                         // bool isVarArg            // 是否为变长参数
    llvm::Function *func = llvm::Function::Create(  // 根据函数类型构造函数体
        printType,                                  // FunctionType *Ty      // 函数类型
        llvm::Function::ExternalLinkage,  // LinkageTypes Linkage  // 定义外部链接
        llvm::Twine("printf"),            // const Twine &N        // 函数名称
        context->module);                 // Module *M             // 装载的 module
    func->setCallingConv(llvm::CallingConv::C);  // 设置调用常数
    return func;
}

int getRecordIndex(
    tree::Type *recType, std::string name)  // 对于记录类型，寻找某一个变量的 index
{
    for (int i = 0; i < recType->child_type.size(); i++) {
        if (name == recType->child_type[i]->name) {
            return i;
        }
    }
    std::cout << "[Error] Unknown name: " << name << " (in record: " << recType->name << ")"
              << std::endl;
    exit(0);
}

llvm::Value *getArrRef(
    tree::BinaryExp *exp, CodeGenContext *context)  // 获得一个 llvm 数组某下标的地址
{
    std::string arrName = static_cast<tree::VariableExp *>(exp->operand1)->name;

    if (exp->operand1->node_type == N_VARIABLE_EXP) {  // 如果二元表达式第一个参数是变量表达式
        std::vector<llvm::Value *> arrIdx(2);
        arrIdx[0] = llvm::ConstantInt::get(
            MyContext, llvm::APInt(32, 0, true));  // 构建 llvm 整型常数，值为 0
        arrIdx[1] = exp->operand2->codeGen(context);  //  获取二元表达式的第二个变量的值

        return llvm::GetElementPtrInst::CreateInBounds(
            context->getValue(
                arrName),  // Value *Ptr                // 从 context 中找到数组变量
            llvm::ArrayRef<llvm::Value *>(arrIdx),  // ArrayRef<Value *> IdxList
            llvm::Twine("tempname"),                // const Twine &NameStr
            context->getCurBlock());                // BasicBlock *InsertAtEnd
    } else {
        std::cout << "[Error] Wrong array type: " << arrName << std::endl;
        exit(0);
    }
}

/* ================== code generate =================== */

llvm::Value *tree::Body::codeGen(CodeGenContext *context) {
    for (tree::Stm *stm : this->stms) {
        stm->codeGen(context);
        std::cout << "[Success] A statment generated" << std::endl;
    }
    return nullptr;
}

llvm::Value *tree::Program::codeGen(CodeGenContext *context) {
    llvm::Value *lastDef = nullptr;

    if (this->define != nullptr) {
        for (tree::ConstDef *conDef : this->define->const_def) {  // 全局常量定义
            lastDef = conDef->codeGen(context);
            std::cout << "[Success] Program defined a const: " << conDef->name << std::endl;
        }

        for (tree::VarDef *varDef : this->define->var_def) {
            varDef->is_global = true;
            lastDef           = varDef->codeGen(context);
            std::cout << "[Success] Program defined a gloable variable: " << varDef->name
                      << std::endl;
        }

        for (tree::FunctionDef *funDef : this->define->function_def) {
            lastDef = funDef->codeGen(context);
            std::cout << "[Success] Program defined a function: " << funDef->name
                      << std::endl;
        }
    }

    this->body->codeGen(context);
    std::cout << "[Success] Program generated" << std::endl;

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
    if (this->value->node_type == N_CONSTANT_EXP) {  // 如果值是常量
        tree::ConstantExp *opLeft = static_cast<ConstantExp *>(this->value);  //
        llvm::Value *alloc        = new llvm::AllocaInst(  // 为常量分配空间
            context->getLLVMTy(opLeft->return_type),  // Type *Ty                 // 类型
            0,                                        // unsigned AddrSpace
            llvm::Twine(this->name),  // const Twine &Name        // 常量名称
            context->getCurBlock());  // BasicBlock *InsertAtEnd
        llvm::Value *store        = new llvm::StoreInst(  // 将常量的值赋到空间中
            opLeft->codeGen(context),  // Value *Val              // 常量的值
            alloc,                     // Value *Ptr              // 常量的地址
            false,                     // bool isVolatile         // 不可更改
            context->getCurBlock());   // BasicBlock InsertAtEnd

        context->insertConst(this->name, this->value);  // 添加常量到上下文中
        std::cout << "[Success] Const defined: " << this->name << std::endl;
        return store;
    } else {
        std::cout << "[Error] Wrong left value(not a const): " << this->name << std::endl;
        exit(0);
    }
}

llvm::Value *tree::TypeDef::codeGen(CodeGenContext *context) {
    context->getCurCodeGenBlock()->typedefs[this->name] = this->type;  // 定义类型
}

llvm::Value *tree::VarDef::codeGen(CodeGenContext *context) {
    std::cout << "Variable defining: " << this->name << std::endl;

    llvm::Value *alloc;
    if (this->is_global) {  // 如果是全局变量
        std::cout << "|--- Global variable" << std::endl;

        if (this->type->base_type == TY_ARRAY) {  // 如果是数组
            std::cout << " |---|--- Array variable" << std::endl;

            std::vector<llvm::Constant *> vec = std::vector<llvm::Constant *>();
            llvm::Constant *eleOfArr;
            switch (this->type->child_type[0]->base_type) {  // 对全局变量进行初始化
                case TY_INTEGER:
                    eleOfArr =
                        llvm::ConstantInt::get(llvm::Type::getInt32Ty(MyContext), 0, true);
                    break;
                case TY_REAL:
                    eleOfArr = llvm::ConstantFP ::get(llvm::Type::getFloatTy(MyContext), 0);
                    break;
                case TY_CHAR:
                    eleOfArr =
                        llvm::ConstantInt::get(llvm::Type::getInt8Ty(MyContext), 0, true);
                    break;
                case TY_BOOLEAN:
                    eleOfArr =
                        llvm::ConstantInt::get(llvm::Type::getInt1Ty(MyContext), 0, true);
                    break;
                default:
                    std::cout << "[Warning] Uncomplete feature for global array of record"
                              << std::endl;
                    exit(0);
            }

            for (int i = 0; i < this->type->array_end - this->type->array_start + 1; i++) {
                vec.push_back(eleOfArr);
            }

            llvm::ArrayType *arrType =
                static_cast<llvm::ArrayType *>(context->getLLVMTy(this->type));
            llvm::Constant *arrConst = llvm::ConstantArray::get(arrType, vec);
            alloc = new llvm::GlobalVariable(*context->module,  // Module &M
                context->getLLVMTy(this->type),  // Type *Ty               // 全局变量的类型
                false,                           // bool isConstant        // 是否常量
                llvm::GlobalValue::ExternalLinkage,  // LinkageTypes Linkage
                arrConst,  // Constant *Initializer  // 全局变量初始化为 0
                llvm::Twine(this->name));  // const Twine &Name      // 变量名称
        } else if (this->type->base_type == TY_RECORD) {
            std::cout << "[Warning] Uncomplete feature for gloable record" << std::endl;
            exit(0);
        } else {  // 不是数组
            llvm::Constant *arrConst;
            switch (this->type->base_type) {
                case TY_INTEGER:
                    arrConst =
                        llvm::ConstantInt::get(llvm::Type::getInt32Ty(MyContext), 0, true);
                    break;
                case TY_REAL:
                    arrConst = llvm::ConstantFP ::get(llvm::Type::getFloatTy(MyContext), 0);
                    break;
                case TY_CHAR:
                    arrConst =
                        llvm::ConstantInt::get(llvm::Type::getInt8Ty(MyContext), 0, true);
                    break;
                case TY_BOOLEAN:
                    arrConst =
                        llvm::ConstantInt::get(llvm::Type::getInt1Ty(MyContext), 0, true);
                    break;
                default:;
            }

            alloc = new llvm::GlobalVariable(*context->module, context->getLLVMTy(this->type),
                false, llvm::GlobalValue::ExternalLinkage, arrConst, llvm::Twine(this->name));
        }
    } else {                                 // 局部变量
        alloc = new llvm::AllocaInst(        // 为局部变量分配地址
            context->getLLVMTy(this->type),  // 类型
            0,
            llvm::Twine(this->name),  // 常量名称
            context->getCurBlock());
    }
    std::cout << "[Success] Variable defined: " << this->name << std::endl;
    return alloc;
}

llvm::Value *tree::FunctionDef::codeGen(CodeGenContext *context) {
    std::cout << "Function defining: " << this->name << std::endl;
    context->defined_functions[this->name] = this;

    std::vector<llvm::Type *> argTy;
    for (int i = 0; i < this->args_type.size(); i++) {
        if (this->args_is_formal_parameters[i]) {  // 参数传递 引用/值
            argTy.push_back(llvm::Type::getInt32PtrTy(MyContext));
        } else {
            argTy.push_back(context->getLLVMTy(this->args_type[i]));
        }
    }

    llvm::FunctionType *funcType = llvm::FunctionType::get(
        context->getLLVMTy(this->rtn_type),  // Type *Ty                 // 函数返回值类型
        llvm::makeArrayRef(argTy),           // ArrayRef<Type *> Params  // 参数类型
        false);                              // bool isVarArg            // 定长
    llvm::Function *func =
        llvm::Function::Create(funcType, llvm::GlobalValue::InternalLinkage,
            llvm::Twine(this->name),  // 函数名
            context->module);

    llvm::BasicBlock *block = llvm::BasicBlock::Create(MyContext, llvm::Twine("entry"), func,
        nullptr);  // BasicBlock *InsertBefore

    llvm::Function *oldFunc = context->getCurFunc();
    context->setCurFunc(func);
    llvm::BasicBlock *oldBlock = context->getCurBlock();
    context->parent[func]      = oldFunc;

    context->pushBlock(block);

    llvm::Value *argValue;
    llvm::Argument *argValue_iter = func->arg_begin();
    for (int i = 0; i < argTy.size(); i++) {
        llvm::Type *ty;
        if (this->args_is_formal_parameters[i]) {
            ty = llvm::Type::getInt32PtrTy(MyContext);
            std::cout << "|--- Formal argument define: " << this->args_name[i] << std::endl;
        } else {
            ty = context->getLLVMTy(this->args_type[i]);
            std::cout << "|--- Argument define: " << this->args_name[i] << std::endl;
        }
        llvm::Value *alloc = new llvm::AllocaInst(  // 为参数分配空间
            ty,                                     // 参数类型
            0,
            llvm::Twine(this->args_name[i]),  // 参数名
            context->getCurBlock());
        argValue           = argValue_iter++;
        argValue->setName(llvm::Twine(this->args_name[i]));
        new llvm::StoreInst(  // 存参数值
            argValue,         // 参数值
            alloc,            // 参数地址
            false,            // 非常量
            block);
    }

    if (this->rtn_type != nullptr) {
        new llvm::AllocaInst(  // 为返回值分配地址
            context->getLLVMTy(this->rtn_type), 0, llvm::Twine(this->name),
            context->getCurBlock());
        std::cout << "|--- Function return value declaration" << std::endl;
    }

    std::cout << "|--- [Success] Function header part generated success!" << std::endl;

    if (this->define != nullptr) {
        std::cout << "|--- Function variable define" << std::endl;
        for (tree::VarDef *var_d : this->define->var_def) {
            var_d->codeGen(context);
        }

        for (tree::FunctionDef *func_d : this->define->function_def) {
            func_d->codeGen(context);
        }

        std::cout << "|--- [Success] Function define part generated success!" << std::endl;
    }

    this->body->codeGen(context);

    if (this->rtn_type != nullptr) {
        std::cout << "|--- Generating return value for function" << std::endl;
        llvm::Value *load = new llvm::LoadInst(  // 加载返回值的地址
            context->getValue(this->name), llvm::Twine(""), false, context->getCurBlock());
        llvm::ReturnInst::Create(MyContext, load, context->getCurBlock());
        std::cout << "|--- Function returned" << std::endl;
    } else {
        std::cout << "|--- Generating return void for procedure" << std::endl;
        llvm::ReturnInst::Create(MyContext, context->getCurBlock());
        std::cout << "|--- Procedure returned" << std::endl;
    }

    while (context->getCurBlock() != oldBlock) {  // 函数定义完成
        context->popBlock();
    }
    context->setCurFunc(oldFunc);

    std::cout << "[Success] Defined function: " << this->name << std::endl;
    return func;
}

llvm::Value *tree::AssignStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating assignment statment..." << std::endl;

    if (this->left_value->node_type == N_BINARY_EXP) {  // 如果左值为二元表达式
        tree::BinaryExp *op1 = static_cast<tree::BinaryExp *>(this->left_value);
        if (op1->op_code == OP_INDEX) {                         // 为取数组下标
            llvm::Value *elementPtr = getArrRef(op1, context);  // 取得目标地址
            std::cout << "[Success] Assignment statment generate" << std::endl;
            return new llvm::StoreInst(this->right_value->codeGen(context),  // 值
                elementPtr,                                                  // 地址
                false, context->getCurBlock());
        } else if (op1->op_code == OP_DOT) {
            // TODO
        } else {
            std::cout << "[Error] Wrong left value type" << std::endl;
            exit(0);
        }
    } else if (this->left_value->node_type == N_VARIABLE_EXP) {  // 如果左值为变量
        tree::VariableExp *op1 = static_cast<tree::VariableExp *>(this->left_value);
        if (op1->codeGen(context)->getType()->isArrayTy()) {  // 如果左值是数组类型
            std::cout << "[Error] Wrong left value type" << std::endl;
            exit(0);
        } else {  // 如果左值不为数组类型
            llvm::Value *tmp = context->getValue(op1->name);
            llvm::Value *load;
            do {
                load = tmp;
                tmp = new llvm::LoadInst(tmp, llvm::Twine(""), false, context->getCurBlock());
            } while (tmp->getType()->isPointerTy());

            return new llvm::StoreInst(this->right_value->codeGen(context),  // 值（右）
                load,  // 地址（左）
                false, context->getCurBlock());
        }
    } else {  // 如果左值不为变量/二元表达式
        std::cout << "[Error] Wrong left value type" << std::endl;
        exit(0);
    }
}

llvm::Value *tree::CallStm::codeGen(CodeGenContext *context) {
    std::cout << "Calling function: " << this->name << std::endl;
    if (this->name == "write" || this->name == "writeln") {  // 对 write 进行特判
        bool isWriteln = (this->name == "writeln");
        std::string printf_format;
        std::vector<llvm::Value *> printfArgs;

        for (tree::Exp *arg : this->args) {
            llvm::Value *argValue = arg->codeGen(context);  // 得到参数的值
            if (argValue->getType() == llvm::Type::getInt32Ty(MyContext)) {
                printf_format += "%d";
                std::cout << "|--- System call write variable previous name"
                          << argValue->getName().str() << std::endl;
                printfArgs.push_back(argValue);
            } else if (argValue->getType()->isFloatTy()) {
                printf_format += "%f";
                printfArgs.push_back(argValue);
            } else if (argValue->getType() == llvm::Type::getInt8PtrTy(MyContext)) {
                std::cout << "[Warning] string print is not supported" << std::endl;
                exit(0);
            } else {
                std::cout << "[Error] Unknown type for printf" << std::endl;
            }
        }

        if (isWriteln) {
            printf_format += "\n";
        }

        std::cout << "print format: " << printf_format << std::endl;
        llvm::Constant *printf_format_const =
            llvm::ConstantDataArray::getString(  // 创建常量 char*
                MyContext, printf_format.c_str());
        llvm::GlobalVariable *format_string_var =
            new llvm::GlobalVariable(  // 创建一个全局变量
                *context->module,
                llvm::ArrayType::get(  // 变量类型 char*
                    llvm::IntegerType::getInt8Ty(MyContext),
                    strlen(printf_format.c_str()) + 1),
                true,  // 常量
                llvm::GlobalValue::PrivateLinkage,
                printf_format_const,  // 变量的值
                llvm::Twine(".str"));
        llvm::Constant *zero = llvm::Constant::getNullValue(  // 创建一个 0
            llvm::IntegerType::getInt32Ty(MyContext));
        std::vector<llvm::Constant *> arrIdx;
        arrIdx.push_back(zero);
        arrIdx.push_back(zero);
        llvm::Constant *var_ref = llvm::ConstantExpr::getGetElementPtr(
            llvm::ArrayType::get(
                llvm::IntegerType::getInt8Ty(MyContext), strlen(printf_format.c_str()) + 1),
            format_string_var,  // 全局变量
            arrIdx);            // 从 0 开始
        printfArgs.insert(printfArgs.begin(), var_ref);
        std::cout << "[Success] Print call generated" << std::endl;
        return llvm::CallInst::Create(context->printf, llvm::makeArrayRef(printfArgs),
            llvm::Twine(""), context->getCurBlock());
    }
    // if(name == "write" || name == "writeln"){
    //   bool writeln = false;
    //   if(name == "writeln")
    //     writeln = true;
    //   std::string printf_format;
    //   std::vector<llvm::Value *> printf_args;
    //
    //   for(auto arg :args) {
    //       llvm::Value* arg_val = arg->codeGen(context);
    //       if (arg_val->getType() == llvm::Type::getInt32Ty(MyContext)) {
    //           printf_format += "%d";
    //           std::cout << "SysFuncCall write variable previous name" <<
    //           arg_val->getName().str() << std::endl; printf_args.push_back(arg_val);
    //       } else if (arg_val->getType()->isFloatTy()) {
    //           printf_format += "%f";
    //           printf_args.push_back(arg_val);
    //       } else if (arg_val->getType() == llvm::Type::getInt8PtrTy(MyContext)) {
    //           std::cout << "string print is not supported" << std::endl;
    //       }
    //   }
    //
    //   if (writeln)
    //       printf_format += "\n";
    //
    //   std::cout << "Desierdsaefad Format: " << printf_format << std::endl;
    //
    //   llvm::Constant* printf_format_const = llvm::ConstantDataArray::getString(MyContext,
    //   printf_format.c_str());
    //
    //   llvm::GlobalVariable* format_string_var = new
    //   llvm::GlobalVariable(*context->module,llvm::ArrayType::get(llvm::IntegerType::get(MyContext,
    //   8),strlen(printf_format.c_str())+ 1),true, llvm::GlobalValue::PrivateLinkage,
    //   printf_format_const, ".str");
    //
    //   llvm::Constant* zero =
    //   llvm::Constant::getNullValue(llvm::IntegerType::getInt32Ty(MyContext));
    //   std::vector<llvm::Constant *> arrIdx;
    //   arrIdx.push_back(zero);
    //   arrIdx.push_back(zero);
    //
    //   llvm::Constant* var_ref =
    //   llvm::ConstantExpr::getGetElementPtr(llvm::ArrayType::get(llvm::IntegerType::get(MyContext,
    //   8),strlen(printf_format.c_str())+ 1),format_string_var,arrIdx);
    //
    //   printf_args.insert(printf_args.begin(), var_ref);
    //   auto call = llvm::CallInst::Create(context->printf, llvm::makeArrayRef(printf_args),
    //   "", context->getCurBlock());
    //   //llvm::ReturnInst::Create(llvm::getGlobalContext(), block);
    //   std::cout << "write or writeln called" << std::endl;
    //   //context->popBlock();
    //   return call;
    // }

    // 非 write/writeln
    llvm::Function *func = context->module->getFunction(this->name.c_str());  // 找到这个函数
    if (func == nullptr) {
        std::cout << "[Error] Function not defined" << std::endl;
        exit(0);
    }
    std::vector<llvm::Value *> argValues;
    auto funcArgs_iter = func->arg_begin();
    for (tree::Exp *arg : this->args) {
        llvm::Value *funcArgValue = static_cast<llvm::Value *>(funcArgs_iter++);
        if (funcArgValue->getType()->isPointerTy()) {  // 如果这个参数是指针（全局变量）
            if (arg->node_type == N_VARIABLE_EXP) {  // 如果这个参数是变量
                llvm::Value *ptr = context->getValue(
                    static_cast<tree::VariableExp *>(arg)->name);  // 取得变量的值
                while (ptr->getType() != llvm::Type::getInt32PtrTy(MyContext)) {
                    ptr = new llvm::LoadInst(
                        ptr, llvm::Twine(""), false, context->getCurBlock());
                }
                argValues.push_back(ptr);
            } else if (arg->node_type == N_BINARY_EXP) {
                BinaryExp *node = static_cast<BinaryExp *>(arg);
                if (node->op_code == OP_DOT) {  // 记录类型
                    if (node->operand2->node_type == N_VARIABLE_EXP) {
                        VariableExp *op2 = static_cast<VariableExp *>(node->operand2);
                        int index = getRecordIndex(node->operand1->return_type, op2->name);
                        std::vector<llvm::Value *> arrIdx(2);
                        arrIdx[0]        = llvm::ConstantInt::get(  // 0
                            MyContext, llvm::APInt(32, 0, true));
                        arrIdx[1]        = llvm::ConstantInt::get(  // index
                            MyContext, llvm::APInt(32, index, true));
                        llvm::Value *ptr = llvm::GetElementPtrInst::Create(
                            context->getLLVMTy(node->operand1->return_type),  // 类型
                            context->getValue(op2->name),                     // 值
                            arrIdx,           // 记录所存在的序号
                            llvm::Twine(""),  // 名称
                            context->getCurBlock());
                        argValues.push_back(ptr);
                    } else {
                        std::cout
                            << "[Error] operand2 of dot operation is not a variable exp type"
                            << std::endl;
                        exit(0);
                    }
                } else if (node->op_code == OP_INDEX) {  // 数组类型
                    if (node->operand1->node_type == N_VARIABLE_EXP) {
                        VariableExp *op1 = static_cast<VariableExp *>(node->operand1);
                        std::vector<llvm::Value *> arrIdx(2);
                        arrIdx[0] =
                            llvm::ConstantInt::get(MyContext, llvm::APInt(32, 0, true));
                        arrIdx[1]        = node->operand2->codeGen(context);
                        llvm::Value *ptr = llvm::GetElementPtrInst::CreateInBounds(
                            context->getValue(op1->name),
                            llvm::ArrayRef<llvm::Value *>(arrIdx), llvm::Twine("tempname"),
                            context->getCurBlock());
                        argValues.push_back(ptr);
                    } else {
                        std::cout << "[Error] Array's Ref is not an array type variable"
                                  << std::endl;
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
    return llvm::CallInst::Create(
        func, llvm::makeArrayRef(argValues), llvm::Twine(""), context->getCurBlock());
}

llvm::Value *tree::LabelStm::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::IfStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating if statment" << std::endl;
    llvm::Value *cond = condition->codeGen(context);
    std::cout << "|--- [Success] Condition generated" << std::endl;
    llvm::BasicBlock *trueBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("then"), context->getCurFunc());
    llvm::BasicBlock *falseBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("else"), context->getCurFunc());
    llvm::BasicBlock *mergeBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("merge"), context->getCurFunc());

    llvm::Value *ret = llvm::BranchInst::Create(trueBlock,  // BasicBlock *True
        falseBlock,                                         // BasicBlock *False
        cond,                                               // Value *cond
        context->getCurBlock());                            // BasicBlock *InsertAtEnd

    context->pushBlock(trueBlock);
    this->true_do->codeGen(context);
    std::cout << "|--- [Success] True block generated" << std::endl;
    llvm::BranchInst::Create(  // 为真的语句生成完成，回到 merge
        mergeBlock, context->getCurBlock());
    context->popBlock();
    context->pushBlock(falseBlock);
    if (this->false_do != nullptr) {
        this->false_do->codeGen(context);
        std::cout << "|--- [Success] False block generated" << std::endl;
    }
    llvm::BranchInst::Create(  // 为假的语句生成完成，回到 merge
        mergeBlock, context->getCurBlock());
    context->popBlock();
    context->pushBlock(mergeBlock);
    std::cout << "[Success] If statment generated" << std::endl;
    return ret;
}

llvm::Value *tree::CaseStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating case statment" << std::endl;
    llvm::BasicBlock *exitBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("exit"), context->getCurFunc());
    std::vector<llvm::BasicBlock *> blocks;
    std::cout << "|--- Contains :" << situations.size() << "cases" << std::endl;

    llvm::Value *ret;
    for (int i = 0; i < this->situations.size(); i++) {
        for (int j = 0; j < this->situations[i]->match_list.size(); j++) {
            llvm::BasicBlock *basBlock = llvm::BasicBlock::Create(
                MyContext, llvm::Twine("case"), context->getCurFunc());

            std::cout << "|--- In case " << i << ":" << j << std::endl;
            tree::BinaryExp *cond = new tree::BinaryExp(
                OP_EQUAL, this->object, this->situations[i]->match_list[j]);

            if (i == this->situations.size() - 1 &&
                j == this->situations[i]->match_list.size() - 1) {  // 最后一块
                ret = llvm::BranchInst::Create(  // 最后一块连接到 exitBlock
                    basBlock,                    // BasicBlock *IfTrue
                    exitBlock,  // BasicBlock *IfFalse      // 不符合条件，退出
                    cond->codeGen(context),   // Value Cond
                    context->getCurBlock());  // BasicBlock *InsertAtEnd
            } else {
                llvm::BasicBlock *nextBlock = llvm::BasicBlock::Create(
                    MyContext, llvm::Twine("next"), context->getCurFunc());
                llvm::BranchInst::Create(
                    basBlock,   // BasicBlock *IfTrue       // 符合条件，这个
                    nextBlock,  // BasicBlock *IfFalse      // 不符合条件，下一个
                    cond->codeGen(context),   // Value Cond
                    context->getCurBlock());  // BasicBlock *InsertAtEnd
                context->pushBlock(nextBlock);
            }
        }
    }

    for (int i = 0; i < this->situations.size(); i++) {  // 对于每一种 situation
        for (int j = 0; j < this->situations[i]->match_list.size();
             j++) {  // 每一种 situation 中的每一个 match
            llvm::BasicBlock *basBlock = llvm::BasicBlock::Create(
                MyContext, llvm::Twine("caseStmt"), context->getCurFunc());
            blocks.push_back(basBlock);  // 建立一个 block 并 push
        }
    }

    // llvm::Value *ret;
    for (int i = 0, p = 0; i < this->situations.size(); i++, p++) {
        for (int j = 0; j < this->situations[i]->match_list.size(); j++, p++) {
            std::cout << "in case No." << i << std::endl;
            std::cout << "|__case's No." << j << std::endl;

            tree::BinaryExp *cond = new tree::BinaryExp(
                OP_EQUAL, this->object, this->situations[i]->match_list[j]);

            llvm::BasicBlock *nextBlock;
            if (p == blocks.size() - 1) {
                ret = llvm::BranchInst::Create(blocks[p],  // BasicBlock *IfTrue
                    exitBlock,                             // BasicBlock *IfFalse
                    cond->codeGen(context),                // Value Cond
                    context->getCurBlock());               // BasicBlock *InsertAtEnd
            } else {
                nextBlock =
                    llvm::BasicBlock::Create(MyContext, "next", context->getCurFunc());

                llvm::BranchInst::Create(blocks[p],  // BasicBlock *IfTrue
                    nextBlock,                       // BasicBlock *IfFalse
                    cond->codeGen(context),          // Value Cond
                    context->getCurBlock());         // BasicBlock *InsertAtEnd

                context->pushBlock(nextBlock);
            }
        }
    }

    for (int i = 0, p = 0; i < this->situations.size(); i++, p++) {
        for (int j = 0; j < this->situations[i]->match_list.size(); j++, p++) {
            context->pushBlock(blocks[p]);
            this->situations[i]->codeGen(context);
            llvm::BranchInst::Create(exitBlock, context->getCurBlock());
            std::cout << "|--- [Success] In case " << i << ":" << j << std::endl;
            context->popBlock();
        }
    }

    context->pushBlock(exitBlock);

    return ret;
}

llvm::Value *tree::ForStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating for statement" << std::endl;
    llvm::BasicBlock *startBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("start"), context->getCurFunc());
    llvm::BasicBlock *loopBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("loop"), context->getCurFunc());
    llvm::BasicBlock *exitBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("exit"), context->getCurFunc());
    // 定义循环变量
    tree::VariableExp *loopVar   = new tree::VariableExp(this->iter);
    loopVar->return_type         = tree::findVar(this->iter, this);
    loopVar->name                = this->iter;
    tree::AssignStm *initLoopVar = new tree::AssignStm(  // 为循环变量赋初值
        loopVar, this->start);
    initLoopVar->codeGen(context);  // 生成赋初值的代码 (i := start)
    llvm::BranchInst::Create(       // 跳转入 startBlock
        startBlock, context->getCurBlock());
    // 循环过程
    context->pushBlock(startBlock);              // startBlock
    tree::BinaryExp *cmp = new tree::BinaryExp(  // 判断循环变量是否到达终点值
        OP_EQUAL, loopVar, this->end);
    llvm::Instruction *ret =
        llvm::BranchInst::Create(exitBlock,  // 到达终点值，跳入 exitBlock
            loopBlock,                       // 未到达终点值，跳入 loopBlock
            cmp->codeGen(context), context->getCurBlock());
    context->popBlock();
    context->pushBlock(loopBlock);  // loopBlock
    this->loop->codeGen(context);   // 生成代码块
    // 循环变量的更新
    tree::BinaryExp *update;
    tree::Value *tmp               = new tree::Value();
    tmp->base_type                 = TY_INTEGER;
    tmp->val.integer_value         = this->step;  // 1 为 to ， -1 为 downto
    tree::ConstantExp *int1        = new tree::ConstantExp(tmp);
    update                         = new tree::BinaryExp(  // i + 1 或 i - 1
        OP_ADD, loopVar, int1);
    tree::AssignStm *updateLoopVar = new tree::AssignStm(  // 更新循环变量的语句
        loopVar, update);
    updateLoopVar->codeGen(context);  // i := i + 1 或 i := i - 1
    llvm::BranchInst::Create(         // 跳入 startBlock
        startBlock, context->getCurBlock());
    context->popBlock();
    context->pushBlock(exitBlock);  // exitBlock
    this->loop->codeGen(context);   // 最后一次循环
    std::cout << "[Success] For loop generated" << std::endl;
    return ret;
}

llvm::Value *tree::WhileStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating while statement" << std::endl;
    llvm::BasicBlock *startBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("start"), context->getCurFunc());
    llvm::BasicBlock *loopBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("loop"), context->getCurFunc());
    llvm::BasicBlock *exitBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("exit"), context->getCurFunc());
    llvm::BranchInst::Create(  // 首先跳转入 startBlock
        startBlock, context->getCurBlock());
    context->pushBlock(startBlock);                         // startBlock
    llvm::Value *ret = llvm::BranchInst::Create(loopBlock,  // 符合 cond ，继续 loopBlock
        exitBlock,  // 不符合 cond ，跳入 exitBlock
        this->condition->codeGen(context), context->getCurBlock());
    context->popBlock();
    context->pushBlock(loopBlock);  // loopBlock
    this->loop->codeGen(context);   // 生成代码块
    llvm::BranchInst::Create(       // 继续循环，跳入 startBlock
        startBlock, context->getCurBlock());
    context->popBlock();
    context->pushBlock(exitBlock);  // exitBlock
    std::cout << "[Success] While loop generated" << std::endl;
    return ret;
}

llvm::Value *tree::RepeatStm::codeGen(CodeGenContext *context) {
    std::cout << "Creating repeat statement" << std::endl;
    llvm::BasicBlock *loopBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("loop"), context->getCurFunc());
    llvm::BasicBlock *exitBlock =
        llvm::BasicBlock::Create(MyContext, llvm::Twine("exit"), context->getCurFunc());
    llvm::BranchInst::Create(  // 跳入 loopBlock
        loopBlock, context->getCurBlock());
    context->pushBlock(loopBlock);                               // loopBlock
    this->loop->codeGen(context);                                // 生成代码块
    llvm::Value *cond      = this->condition->codeGen(context);  // 跳转条件
    llvm::Instruction *ret = llvm::BranchInst::Create(exitBlock,  // 符合 until 条件，退出
        loopBlock,  // 不符合，跳回 loopBlock
        cond, context->getCurBlock());
    context->popBlock();
    context->pushBlock(exitBlock);  // exitBlock
    std::cout << "[Success] Repeat loop generated" << std::endl;
    return ret;
}

llvm::Value *tree::GotoStm::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::UnaryExp::codeGen(CodeGenContext *context) {
    switch (this->op_code) {
        case OP_NOT:
            return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT,
                this->operand->codeGen(context),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(MyContext), 0, true),
                llvm::Twine(""), context->getCurBlock());
        case OP_OPPO:
            if (this->operand->return_type->base_type == TY_INTEGER) {  // -m = 0 - m
                return llvm::BinaryOperator::Create(llvm::Instruction::Sub,
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(MyContext), 0, true),
                    this->operand->codeGen(context), llvm::Twine(""), context->getCurBlock());
            } else if (this->operand->return_type->base_type == TY_REAL) {
                return llvm::BinaryOperator::Create(llvm::Instruction::FSub,
                    llvm::ConstantFP::get(MyContext, llvm::APFloat(0.)),
                    this->operand->codeGen(context), llvm::Twine(""), context->getCurBlock());
            }
            break;
        case OP_ABS:
            if (this->operand->return_type->base_type == TY_INTEGER) {
                llvm::Value *cond          = llvm::CmpInst::Create(llvm::Instruction::ICmp,
                    llvm::CmpInst::ICMP_SGT,
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(MyContext), 0, true),
                    this->operand->codeGen(context), llvm::Twine(""), context->getCurBlock());
                llvm::BasicBlock *negBlock = llvm::BasicBlock::Create(
                    MyContext, llvm::Twine("neg"), context->getCurFunc());
                llvm::BasicBlock *posBlock = llvm::BasicBlock::Create(
                    MyContext, llvm::Twine("pos"), context->getCurFunc());
                llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(
                    MyContext, llvm::Twine("merge"), context->getCurFunc());
                llvm::Value *ret = llvm::BranchInst::Create(negBlock,  // < 0, negBlock
                    posBlock,                                          // > 0, posBlock
                    cond, context->getCurBlock());
                context->pushBlock(negBlock);  // negBlock
                llvm::BinaryOperator::Create(  // abs(m) = -m
                    llvm::Instruction::Sub,
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(MyContext), 0, true),
                    this->operand->codeGen(context), llvm::Twine(""), context->getCurBlock());
                llvm::BranchInst::Create(  // 跳入 mergeBlock
                    mergeBlock, context->getCurBlock());
                context->popBlock();
                context->pushBlock(posBlock);     // faseBlock
                this->operand->codeGen(context);  // abs(m) = m
                llvm::BranchInst::Create(         // 跳入 mergeBlock
                    mergeBlock, context->getCurBlock());
                context->popBlock();
                context->pushBlock(mergeBlock);  // mergeBlock
                return ret;
            } else if (this->operand->return_type->base_type == TY_REAL) {
                llvm::Value *cond          = llvm::CmpInst::Create(llvm::Instruction::ICmp,
                    llvm::CmpInst::ICMP_SGT,
                    llvm::ConstantFP::get(MyContext, llvm::APFloat(0.)),
                    this->operand->codeGen(context), llvm::Twine(""), context->getCurBlock());
                llvm::BasicBlock *negBlock = llvm::BasicBlock::Create(
                    MyContext, llvm::Twine("neg"), context->getCurFunc());
                llvm::BasicBlock *posBlock = llvm::BasicBlock::Create(
                    MyContext, llvm::Twine("pos"), context->getCurFunc());
                llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(
                    MyContext, llvm::Twine("merge"), context->getCurFunc());
                llvm::Value *ret = llvm::BranchInst::Create(
                    negBlock, posBlock, cond, context->getCurBlock());
                context->pushBlock(negBlock);
                llvm::BinaryOperator::Create(llvm::Instruction::FSub,
                    llvm::ConstantFP::get(MyContext, llvm::APFloat(0.)),
                    this->operand->codeGen(context), llvm::Twine(""), context->getCurBlock());
                llvm::BranchInst::Create(mergeBlock, context->getCurBlock());
                context->popBlock();
                context->pushBlock(posBlock);
                this->operand->codeGen(context);
                llvm::BranchInst::Create(mergeBlock, context->getCurBlock());
                context->popBlock();
                context->pushBlock(mergeBlock);
                return ret;
            }
        case OP_PRED:
            return llvm::BinaryOperator::Create(  // m - 1
                llvm::Instruction::Sub, this->operand->codeGen(context),
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(MyContext), 1, true),
                llvm::Twine(""), context->getCurBlock());
        case OP_SUCC:
            return llvm::BinaryOperator::Create(  // m + 1
                llvm::Instruction::Add, this->operand->codeGen(context),
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(MyContext), 1, true),
                llvm::Twine(""), context->getCurBlock());
        case OP_ODD:
            return llvm::BinaryOperator::Create(  // 0 & m
                llvm::Instruction::And,
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(MyContext), 0, true),
                this->operand->codeGen(context), llvm::Twine(""), context->getCurBlock());
        case OP_CHR:
            return llvm::CastInst::CreateIntegerCast(this->operand->codeGen(context),
                llvm::Type::getInt8Ty(MyContext), true, llvm::Twine(""),
                context->getCurBlock());
        case OP_ORD:
            return llvm::CastInst::CreateIntegerCast(this->operand->codeGen(context),
                llvm::Type::getInt32Ty(MyContext), true, llvm::Twine(""),
                context->getCurBlock());
    }
}

llvm::Value *tree::BinaryExp::codeGen(CodeGenContext *context) {
    if (this->op_code == OP_DOT) {                          // record
        if (this->operand2->node_type == N_VARIABLE_EXP) {  // 操作符 2 是变量
            tree::VariableExp *op2 = static_cast<tree::VariableExp *>(this->operand2);
            int index = getRecordIndex(  // 找到所求变量在记录中的位置
                this->operand1->return_type, op2->name);
            std::vector<llvm::Value *> arrIdx(2);
            arrIdx[0]           = llvm::ConstantInt::get(  // 0
                MyContext, llvm::APInt(32, 0, true));
            arrIdx[1]           = llvm::ConstantInt::get(            // index
                MyContext, llvm::APInt(32, index, true));  // create member_index
            llvm::Value *memPtr = llvm::GetElementPtrInst::Create(
                context->getLLVMTy(this->operand1->return_type), context->getValue(op2->name),
                arrIdx, llvm::Twine(""), context->getCurBlock());
            return new llvm::LoadInst(memPtr, llvm::Twine(""), false, context->getCurBlock());
        } else {
            std::cout << "[Error] Wrong member variable for record" << std::endl;
            exit(0);
        }
    } else if (this->op_code == OP_INDEX) {
        if (this->operand1->node_type == N_VARIABLE_EXP) {
            tree::VariableExp *op1 = static_cast<tree::VariableExp *>(this->operand1);
            std::vector<llvm::Value *> arrIdx(2);
            arrIdx[0]           = llvm::ConstantInt::get(MyContext, llvm::APInt(32, 0, true));
            arrIdx[1]           = this->operand2->codeGen(context);
            llvm::Value *memPtr = llvm::GetElementPtrInst::CreateInBounds(
                context->getValue(op1->name), llvm::ArrayRef<llvm::Value *>(arrIdx),
                llvm::Twine("tempname"), context->getCurBlock());
            return new llvm::LoadInst(memPtr, llvm::Twine(""), false, context->getCurBlock());
        } else {
            std::cout << "[Error] Array's Ref is not an array type variable" << std::endl;
            exit(0);
        }
    }
    llvm::Value *op1Val = this->operand1->codeGen(context);
    llvm::Value *op2Val = this->operand2->codeGen(context);

    if (op1Val->getType()->isFloatTy() ||
        op2Val->getType()->isFloatTy()) {  // 如果是浮点数运算
        switch (this->op_code) {
            case OP_ADD:
                return llvm::BinaryOperator::Create(llvm::Instruction::FAdd, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_MINUS:
                return llvm::BinaryOperator::Create(llvm::Instruction::FSub, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_MULTI:
                return llvm::BinaryOperator::Create(llvm::Instruction::FMul, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_RDIV:
                return llvm::BinaryOperator::Create(llvm::Instruction::FDiv, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_DDIV:
                return llvm::BinaryOperator::Create(llvm::Instruction::SDiv, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_MOD:
                return llvm::BinaryOperator::Create(llvm::Instruction::SRem, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_AND:
                return llvm::BinaryOperator::Create(llvm::Instruction::And, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_OR:
                return llvm::BinaryOperator::Create(llvm::Instruction::Or, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            // logical
            case OP_SMALL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLT,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_LARGE:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_SMALL_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGE,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_LARGE_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLE,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_EQ,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_NOT_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_NE,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            default:
                std::cout << "[Error] Unknown type of op_code:" << op_code
                          << std::endl;  // not know what to do
                exit(0);
        }
    } else {  // 整型运算
        switch (op_code) {
            case OP_ADD:
                return llvm::BinaryOperator::Create(llvm::Instruction::Add, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_MINUS:
                return llvm::BinaryOperator::Create(llvm::Instruction::Sub, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_MULTI:
                return llvm::BinaryOperator::Create(llvm::Instruction::Mul, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_RDIV:
                return llvm::BinaryOperator::Create(llvm::Instruction::UDiv, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_DDIV:
                return llvm::BinaryOperator::Create(llvm::Instruction::SDiv, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_MOD:
                return llvm::BinaryOperator::Create(llvm::Instruction::SRem, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_AND:
                return llvm::BinaryOperator::Create(llvm::Instruction::And, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_OR:
                return llvm::BinaryOperator::Create(llvm::Instruction::Or, op1Val, op2Val,
                    llvm::Twine(""), context->getCurBlock());
            case OP_SMALL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLT,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_LARGE:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_SMALL_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGE,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_LARGE_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLE,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_EQ,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            case OP_NOT_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_NE,
                    op1Val, op2Val, llvm::Twine(""), context->getCurBlock());
            default:
                std::cout << "[Error] Unknown type of op_code:" << op_code << std::endl;
                exit(0);
        }
    }
}

llvm::Value *tree::CallExp::codeGen(CodeGenContext *context) {
    std::cout << "Creating calling " << std::endl;
    llvm::Function *func = context->module->getFunction(this->name.c_str());
    if (func == nullptr) {
        std::cout << "[Error] Function: " << name << " not defined" << std::endl;
        exit(0);
    }
    std::vector<llvm::Value *> argValues;
    auto funcArgs_iter = func->arg_begin();
    for (tree::Exp *arg : this->args) {
        llvm::Value *funcArgValue = static_cast<llvm::Value *>(funcArgs_iter++);
        if (funcArgValue->getType()->isPointerTy()) {  // 如果这个参数是指针（全局变量）
            if (arg->node_type == N_VARIABLE_EXP) {  // 如果这个参数是变量
                llvm::Value *ptr = context->getValue(
                    static_cast<tree::VariableExp *>(arg)->name);  // 取得变量的值
                while (ptr->getType() != llvm::Type::getInt32PtrTy(MyContext)) {
                    ptr = new llvm::LoadInst(
                        ptr, llvm::Twine(""), false, context->getCurBlock());
                }
                argValues.push_back(ptr);
            } else if (arg->node_type == N_BINARY_EXP) {
                BinaryExp *node = static_cast<BinaryExp *>(arg);
                if (node->op_code == OP_DOT) {  // 记录类型
                    if (node->operand2->node_type == N_VARIABLE_EXP) {
                        VariableExp *op2 = static_cast<VariableExp *>(node->operand2);
                        int index = getRecordIndex(node->operand1->return_type, op2->name);
                        std::vector<llvm::Value *> arrIdx(2);
                        arrIdx[0]        = llvm::ConstantInt::get(  // 0
                            MyContext, llvm::APInt(32, 0, true));
                        arrIdx[1]        = llvm::ConstantInt::get(  // index
                            MyContext, llvm::APInt(32, index, true));
                        llvm::Value *ptr = llvm::GetElementPtrInst::Create(
                            context->getLLVMTy(node->operand1->return_type),  // 类型
                            context->getValue(op2->name),                     // 值
                            arrIdx,           // 记录所存在的序号
                            llvm::Twine(""),  // 名称
                            context->getCurBlock());
                        argValues.push_back(ptr);
                    } else {
                        std::cout
                            << "[Error] operand2 of dot operation is not a variable exp type"
                            << std::endl;
                        exit(0);
                    }
                } else if (node->op_code == OP_INDEX) {  // 数组类型
                    if (node->operand1->node_type == N_VARIABLE_EXP) {
                        VariableExp *op1 = static_cast<VariableExp *>(node->operand1);
                        std::vector<llvm::Value *> arrIdx(2);
                        arrIdx[0] =
                            llvm::ConstantInt::get(MyContext, llvm::APInt(32, 0, true));
                        arrIdx[1]        = node->operand2->codeGen(context);
                        llvm::Value *ptr = llvm::GetElementPtrInst::CreateInBounds(
                            context->getValue(op1->name),
                            llvm::ArrayRef<llvm::Value *>(arrIdx), llvm::Twine("tempname"),
                            context->getCurBlock());
                        argValues.push_back(ptr);
                    } else {
                        std::cout << "[Error] Array's Ref is not an array type variable"
                                  << std::endl;
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
    return llvm::CallInst::Create(
        func, llvm::makeArrayRef(argValues), llvm::Twine(""), context->getCurBlock());
}

llvm::Value *tree::ConstantExp::codeGen(CodeGenContext *context) {
    return value->codeGen(context);
}

llvm::Value *tree::VariableExp::codeGen(CodeGenContext *context) {
    std::cout << "loading variable: " << this->name << std::endl;
    llvm::Value *ptr = context->getValue(this->name);
    ptr = new llvm::LoadInst(ptr, llvm::Twine(""), false, context->getCurBlock());
    if (ptr->getType()->isPointerTy()) {
        ptr = new llvm::LoadInst(ptr, llvm::Twine(""), false, context->getCurBlock());
    }
    return ptr;
}

llvm::Value *tree::Type::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::Value::codeGen(CodeGenContext *context) {
    switch (this->base_type) {
        case TY_INTEGER:
            return llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(MyContext), this->val.integer_value, true);
        case TY_REAL:
            return llvm::ConstantFP::get(MyContext, llvm::APFloat(this->val.real_value));
        case TY_CHAR:
            return llvm::ConstantInt::get(
                llvm::Type::getInt8Ty(MyContext), this->val.char_value, true);
        case TY_BOOLEAN:
            return llvm::ConstantInt::get(
                llvm::Type::getInt1Ty(MyContext), this->val.boolean_value, true);
        default: return nullptr;
    }
}
