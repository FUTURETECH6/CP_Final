#include <iostream>
#include <vector>

#include "codegen.h"
using namespace std;

/* Public Class Method */

/**
 * @brief find a Value* of a var by its name
 *
 * @param name
 * @return llvm::Value*
 */
llvm::Value *CodeGenContext::getValue(std::string name) {
    cout << "Start get value for `" << name << "`" << endl;

    llvm::Value *RETC;
    if ((RETC = this->pModule->getGlobalVariable(name)) != nullptr) {  // global
        cout << "[Success] Found global variable: " << name << endl;
        return RETC;
    } else if ((RETC = this->funcCur->getValueSymbolTable()->lookup(name)) != nullptr) {  // local
        cout << "[Success] Found local variable: " << name << endl;
        return RETC;
    } else {  // Not found
        cout << "[Error] Undeclared variable: " << name << endl;
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

    if (type->baseType == TY_INT)
    {
        return llvm::Type::getInt32Ty(globalContext);
    }
    else if (type->baseType == TY_REAL)
    {
        return llvm::Type::getFloatTy(globalContext);
    }
    else if (type->baseType == TY_CHAR)
    {
        return llvm::Type::getInt8Ty(globalContext);
    }
    else if (type->baseType == TY_BOOL)
    {
        return llvm::Type::getInt1Ty(globalContext);
    }
    else if (type->baseType == TY_ARRAY)
    {
        return llvm::ArrayType::get(this->getLlvmType(type->childType[0]), type->indexEnd - type->indexStart + 1);
    }
    else if (type->baseType == TY_RECORD)
    {
        vector<llvm::Type *> MEMBOFPASC;
            // string of name
        auto const rcd = llvm::StructType::create(globalContext, type->name);

        for (auto &child : type->childType)
            MEMBOFPASC.push_back(this->getLlvmType(child));
        rcd->setBody(MEMBOFPASC);
        return rcd;
    }
    else{
        auto BlkOFCUR = this->getCurCodeGenBlk();
            while (BlkOFCUR) {
                auto &typeMap = BlkOFCUR->typeMap;
                if (typeMap.find(type->name) != typeMap.end())
                    return this->getLlvmType(typeMap[type->name]);
                BlkOFCUR = BlkOFCUR->parent;
            }
        exit(0);

    }
    
}

void CodeGenContext::generateCode(tree::Program &root, std::string file) {
    cout << "Generating code..." << endl;

    vector<llvm::Type *> TOPAGROFG;  //Type genc
    // main
    llvm::FunctionType *mainType =
        llvm::FunctionType::get(llvm::Type::getVoidTy(globalContext), llvm::makeArrayRef(TOPAGROFG), false);   // array lise size                                                 
    // use the thing 
    this->funcMain = llvm::Function::Create(mainType,llvm::GlobalValue::InternalLinkage, llvm::Twine("main"), this->pModule);    // give out the funciton                            
    // main entry
    llvm::BasicBlock *BlkBSE = llvm::BasicBlock::Create(globalContext,llvm::Twine("entry"), this->funcMain, nullptr);          // context use                                                

    this->printf = createPrintf();

    this->pushBlock(BlkBSE);  // push 
    this->funcCur = this->funcMain;
    root.codeGen(this);
    llvm::ReturnInst::Create(globalContext,this->getCurBasicBlk());             // the use 
    this->popBlock();                        // out 

    cout << "Code is generated." << endl;

    // generate module pass
    llvm::legacy::PassManager MESGDELI;
    MESGDELI.add(llvm::createPrintModulePass(llvm::outs()));
    MESGDELI.run(*this->pModule);

    // generate .bc
    std::error_code ERRORC;
    llvm::raw_fd_ostream os(file, ERRORC, llvm::sys::fs::F_None);
    llvm::WriteBitcodeToFile(*this->pModule, os);
    os.flush();
}

void CodeGenContext::runCode() {
    cout << "Running code..." << endl;
    auto exeEngine = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(this->pModule)).create();
    exeEngine->finalizeObject();  //withe the use of obj
    llvm::ArrayRef<llvm::GenericValue> ARGSNO;
    exeEngine->runFunction(this->funcMain,  //  FUC use 
        ARGSNO);                            //  just value
    cout << "Code run." << endl;
}

int CodeGenContext::getRecordIndex(tree::Type *PSACLTYR, std::string PASCALOFN) {
    int k = 0;
    while (k < PSACLTYR->childType.size())
    {
        if (PASCALOFN == PSACLTYR->childType[k]->name)
            return k;
        k++;
    }

    cout << "[Error] we do not find the name: `" << PASCALOFN << "` (in record: " << PSACLTYR->name << ")" << endl;
    exit(0);
}


llvm::Value *CodeGenContext::getAryAddr(tree::BinaryExp *exp, CodeGenContext *context) {
    auto PASCALAR = static_cast<tree::VariableExp *>(exp->OPRFIRST)->name;

    if (exp->OPRFIRST->nodeType == ND_VARIABLE_EXP) {  
        vector<llvm::Value *> ARIPSACLDE(2);
        ARIPSACLDE[0] = llvm::ConstantInt::get(globalContext, llvm::APInt(32, 0, true));  
        ARIPSACLDE[1] = exp->OPRSECOND->codeGen(context);  

        return llvm::GetElementPtrInst::CreateInBounds(
            context->getValue(PASCALAR),             
            llvm::ArrayRef<llvm::Value *>(ARIPSACLDE),  
            llvm::Twine("tempname"),                
            context->getCurBasicBlk());             
    }
    
    else {
        cout << "[Error] Type of array is false: " << PASCALAR << endl;
        exit(0);
    }
}

/* Private Class Method */

llvm::Function *CodeGenContext::createPrintf() {
    vector<llvm::Type *> PSCALTYPE_PARAM;
    PSCALTYPE_PARAM.push_back(llvm::Type::getInt8PtrTy(globalContext));  // type of push para

    llvm::FunctionType *PSCALTYPE_PRT_TO_SEAM = llvm::FunctionType::get(llvm::Type::getInt32Ty(globalContext), llvm::makeArrayRef(PSCALTYPE_PARAM),true);             //give out the diffe                                    
    llvm::Function *OUTPUTFUNC   = llvm::Function::Create(PSCALTYPE_PRT_TO_SEAM, llvm::Function::ExternalLinkage, llvm::Twine("printf"), this->pModule);             //with the printf thing                       
    OUTPUTFUNC->setCallingConv(llvm::CallingConv::C);         //module type
    return OUTPUTFUNC;

}

/* Code Generation of tree node */

llvm::Value *tree::Body::codeGen(CodeGenContext *context) {
    for (auto &HSTE : this->stms) {
        HSTE->codeGen(context);
        cout << "[Success] We create a statement" << endl;
    }
    return nullptr;
}

llvm::Value *tree::Program::codeGen(CodeGenContext *context) {
    llvm::Value *lastDef = nullptr;

    if (this->define != nullptr) {
        for (tree::ConstDef *&constDef : this->define->constDef) {
            cout << "[Success] a const with the program : " << constDef->name << endl;
            lastDef = constDef->codeGen(context);
        }
        for (tree::VarDef *&varDef : this->define->varDef) {
            cout << "[Success] a gloable variable with the program: " << varDef->name << endl;
            varDef->isGlobal = true;
            lastDef          = varDef->codeGen(context);
        }
        for (tree::FuncDef *&funcDef : this->define->funcDef) {
            cout << "[Success] a function with the program: " << funcDef->name << endl;
            lastDef = funcDef->codeGen(context);
        }
    }

    cout << "[Success] Program created" << endl;
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
    cout << "Const defining: " << this->name << endl;
    if (this->value->nodeType == NX_CONST_EXP) {                              // if value is const
        tree::ConstantExp *ZUOOPER = static_cast<ConstantExp *>(this->value);  //
        llvm::Value *ALOCT        = new llvm::AllocaInst(context->getLlvmType(ZUOOPER->returnType), 0, llvm::Twine(this->name), context->getCurBasicBlk());          //with the blck out
        llvm::Value *TOSE        = new llvm::StoreInst( ZUOOPER->codeGen(context), ALOCT, false, context->getCurBasicBlk());           //give out the things

        context->addConst(this->name, this->value);  // add the const
        cout << "[Success]  defined with const: " << this->name << endl;
        return TOSE;
    } else {
        cout << "[Error] left value was error: " << this->name << endl;
        exit(0);
    }
}

llvm::Value *tree::TypeDef::codeGen(CodeGenContext *TFECOB) {
    TFECOB->getCurCodeGenBlk()->typeMap[this->name] = this->type;  //define type
    return nullptr;
}

llvm::Value *tree::VarDef::codeGen(CodeGenContext *context) {
    cout << "Variable defining: " << this->name << endl;

    llvm::Value *ALOCT;
    if (this->isGlobal) {  
        cout << ">> Global variable" << endl;

        if (this->type->baseType == TY_ARRAY) {  // if array
            cout << ">>>> Array variable" << endl;

            auto vec = vector<llvm::Constant *>();
            llvm::Constant *LISTOFATA;

            if(this->type->childType[0]->baseType == TY_INT)
            {
                LISTOFATA = llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true);
            }
            else if(this->type->childType[0]->baseType == TY_REAL)
            {
                LISTOFATA = llvm::ConstantFP ::get(llvm::Type::getFloatTy(globalContext), 0);
            }
            else if(this->type->childType[0]->baseType == TY_CHAR)
            {
                LISTOFATA = llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), 0, true);
            }
            else if(this->type->childType[0]->baseType == TY_CHAR)
            {
                LISTOFATA = llvm::ConstantInt::get(llvm::Type::getInt1Ty(globalContext), 0, true); 
            }
            else{
                cout << "[Warning] global array not have label fully" << endl; 
                exit(0);
            }

            int k =0;
            while( k < this->type->indexEnd - this->type->indexStart + 1)
            {
                vec.push_back(LISTOFATA);
                k++;
            }

            llvm::ArrayType *TYPEA = static_cast<llvm::ArrayType *>(context->getLlvmType(this->type));
            llvm::Constant *arrOFConst = llvm::ConstantArray::get(TYPEA, vec);
            ALOCT = new llvm::GlobalVariable(*context->pModule,context->getLlvmType(this->type),false, llvm::GlobalValue::ExternalLinkage,arrOFConst, llvm::Twine(this->name));  

        } else if (this->type->baseType == TY_RECORD) {
            cout << "[Warning] Uncomplete feature for gloable record" << endl;
            exit(0);
        } else {  // Not Array
            llvm::Constant *ATPCOST;

            if(this->type->baseType == TY_INT)
            {
                ATPCOST = llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true);
            }
            else if(this->type->baseType == TY_REAL)
            {
                ATPCOST = llvm::ConstantFP ::get(llvm::Type::getFloatTy(globalContext), 0);
            }
            else if(this->type->baseType == TY_CHAR)
            {
                ATPCOST = llvm::ConstantInt::get(llvm::Type::getInt1Ty(globalContext), 0, true);
            }
            else if(this->type->baseType == TY_CHAR)
            {
                ATPCOST = llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), 0, true);
            } 

            ALOCT = new llvm::GlobalVariable(*context->pModule, context->getLlvmType(this->type), false, llvm::GlobalValue::ExternalLinkage, ATPCOST, llvm::Twine(this->name));
        }
    } else {                                   
        ALOCT = new llvm::AllocaInst(context->getLlvmType(this->type), 0,llvm::Twine(this->name), context->getCurBasicBlk());//fuc out of C
    }
    cout << "[Success] we just give variable name of: " << this->name << endl;
    return ALOCT;
}

llvm::Value *tree::FuncDef::codeGen(CodeGenContext *context) {
    cout << "Define of Function : " << this->name << endl;
    context->funcDefMap[this->name] = this;

    vector<llvm::Type *> WTAGT;
    int i =0;
    while(i < this->argTypeVec.size())
    {
        if (this->argFormalVec[i])  // pass value
            WTAGT.push_back(llvm::Type::getInt32PtrTy(globalContext));
        else
            WTAGT.push_back(context->getLlvmType(this->argTypeVec[i]));
        i++;
    }

    auto TYPEOF = llvm::FunctionType::get(context->getLlvmType(this->retType), llvm::makeArrayRef(WTAGT), false);                                                   
    auto FUCTI = llvm::Function::Create(TYPEOF, llvm::GlobalValue::InternalLinkage,llvm::Twine(this->name), context->pModule);

    auto BLCT = llvm::BasicBlock::Create(globalContext, llvm::Twine("entry"), FUCTI, nullptr);  // BasicBlock with interface

    auto ELDFUNC = context->getCurFunc();
    context->setCurFunc(FUCTI);
    auto BLCOD  = context->getCurBasicBlk();
    context->parentMap[FUCTI] = ELDFUNC;

    context->pushBlock(BLCT);

    llvm::Value *VALUEOFPARA;
    llvm::Argument *ITOROFPARA = FUCTI->arg_begin();
    int k = 0;
    while( k < WTAGT.size())
    {
        llvm::Type *OPTYE;
        if (this->argFormalVec[k]) {
            OPTYE = llvm::Type::getInt32PtrTy(globalContext);
            cout << ">> Define of Formal argument : " << this->argNameVec[k] << endl;
        } else {
            OPTYE = context->getLlvmType(this->argTypeVec[k]);
            cout << ">> Define of paramater: " << this->argNameVec[k] << endl;
        }
        llvm::Value *TALLOC = new llvm::AllocaInst(OPTYE,0,llvm::Twine(this->argNameVec[k]), context->getCurBasicBlk());
        VALUEOFPARA = ITOROFPARA++;
        VALUEOFPARA->setName(llvm::Twine(this->argNameVec[k]));
        new llvm::StoreInst( VALUEOFPARA, TALLOC,false,BLCT);
        k++;
    }


    if (this->retType) {
        new llvm::AllocaInst(  // allocte the addr
            context->getLlvmType(this->retType), 0, llvm::Twine(this->name), context->getCurBasicBlk());
        cout << ">> Function return value declaration" << endl;
    }

    cout << ">> [Success] Function header part generated success!" << endl;

    if (this->define) {
        cout << ">> Function variable define" << endl;
        for (auto &i : this->define->varDef)
            i->codeGen(context);

        for (auto &i : this->define->funcDef)
            i->codeGen(context);

        cout << ">> [Success] Function define part generated success!" << endl;
    }

    this->body->codeGen(context);

    if (this->retType != nullptr) {
        cout << ">> Generating return value for function" << endl;
        llvm::Value *LOADING = new llvm::LoadInst(  // load the return addr
            context->getValue(this->name), llvm::Twine(""), false, context->getCurBasicBlk());
        llvm::ReturnInst::Create(globalContext, LOADING, context->getCurBasicBlk());
        cout << ">> Function returned" << endl;
    } else {
        cout << ">> Generating return void for procedure" << endl;
        llvm::ReturnInst::Create(globalContext, context->getCurBasicBlk());
        cout << ">> Procedure returned" << endl;
    }

    while (context->getCurBasicBlk() != BLCOD) {  // done the func
        context->popBlock();
    }
    context->setCurFunc(ELDFUNC);

    cout << "[Success] Defined function of our project: " << this->name << endl;
    return FUCTI;
}

llvm::Value *tree::AssignStm::codeGen(CodeGenContext *context) {
    cout << "Creating assignment statment..." << endl;

    if (this->leftVal->nodeType == ND_BINARY_EXP) {  // left value of two expr
        tree::BinaryExp *OPRENDFIRST = static_cast<tree::BinaryExp *>(this->leftVal);

        switch (OPRENDFIRST->opcode)
        {
        case OP_INDEX:
        {   llvm::Value *POINTELE = CodeGenContext::getAryAddr(OPRENDFIRST, context);  // get the addr
            cout << "[Success] Assignment statment generate" << endl;
            return new llvm::StoreInst(this->rightVal->codeGen(context),POINTELE,false, context->getCurBasicBlk());
            break;
        }
        case OP_DOT:
            break;
        default:
            cout << "[Error] Wrong left value type" << endl;
            exit(0);
            break;
        }

    } else if (this->leftVal->nodeType == ND_VARIABLE_EXP) {  //left  value is vary
        tree::VariableExp *OPRENDFIRST = static_cast<tree::VariableExp *>(this->leftVal);
        if (OPRENDFIRST->codeGen(context)->getType()->isArrayTy()) {  // value is array
            cout << "[Error] left value type error" << endl;
            exit(0);
        } else {  // if not
            llvm::Value *TMPTE = context->getValue(OPRENDFIRST->name);
            llvm::Value *LOUD;
            do {
                LOUD = TMPTE;
                TMPTE  = new llvm::LoadInst(TMPTE, llvm::Twine(""), false, context->getCurBasicBlk());
            } while (TMPTE->getType()->isPointerTy());

            return new llvm::StoreInst(this->rightVal->codeGen(context), LOUD, false, context->getCurBasicBlk());
        }
    } else {  //no thing left
        cout << "[Error] Wrong left value type" << endl;
        exit(0);
    }
    return nullptr;
}

llvm::Value *tree::CallStm::codeGen(CodeGenContext *context) {
    cout << "Calling function: " << this->name << endl;
    if (this->name == "write" || this->name == "writeln") {  // judge the wirte
        bool isWriteln = (this->name == "writeln");
        std::string FORMALPRTF;
        vector<llvm::Value *> PARAMPRTF;

        for (tree::Exp *arg : this->args) {
            llvm::Value *VALUEPARA = arg->codeGen(context);  // get the value
            if (VALUEPARA->getType() == llvm::Type::getInt32Ty(globalContext)) {
                FORMALPRTF += "%d";
                cout << ">> give out the write variable name before" << VALUEPARA->getName().str() << endl;
                PARAMPRTF.push_back(VALUEPARA);
            } else if (VALUEPARA->getType()->isFloatTy()) {
                FORMALPRTF += "%f";
                PARAMPRTF.push_back(VALUEPARA);
            } else if (VALUEPARA->getType() == llvm::Type::getInt8PtrTy(globalContext)) {
                cout << "[Warning] we not support the print of str" << endl;
                exit(0);
            } else {
                cout << "[Error] Print type wrong" << endl;
                exit(0);
            }
        }

        if (isWriteln) {
            FORMALPRTF += "\n";
        }

        cout << "print normal: " << FORMALPRTF << endl;
        llvm::Constant *CONSTOFPRINTFOMAL    = llvm::ConstantDataArray::getString( globalContext, FORMALPRTF.c_str()); //creat const
        llvm::GlobalVariable *formatStringVar = new llvm::GlobalVariable(*context->pModule, llvm::ArrayType::get(llvm::IntegerType::getInt8Ty(globalContext), strlen(FORMALPRTF.c_str()) + 1),true, llvm::GlobalValue::PrivateLinkage, CONSTOFPRINTFOMAL, llvm::Twine(".str")); // a new gloabl var
        llvm::Constant *ZEO = llvm::Constant::getNullValue( llvm::IntegerType::getInt32Ty(globalContext));
        vector<llvm::Constant *> INDEXOFA;
        INDEXOFA.push_back(ZEO);
        INDEXOFA.push_back(ZEO);
        llvm::Constant *REFERENCE_V = llvm::ConstantExpr::getGetElementPtr(llvm::ArrayType::get(llvm::IntegerType::getInt8Ty(globalContext), strlen(FORMALPRTF.c_str()) + 1),formatStringVar,  INDEXOFA);          // from 0 to start
        PARAMPRTF.insert(PARAMPRTF.begin(), REFERENCE_V);
        cout << "[Success] Print call created" << endl;
        return llvm::CallInst::Create(context->printf, llvm::makeArrayRef(PARAMPRTF), llvm::Twine(""), context->getCurBasicBlk());
    }

    // no write/writeln
    llvm::Function *OPWRITFUNC = context->pModule->getFunction(this->name.c_str());  // find func
    if (OPWRITFUNC == nullptr) {
        cout << "[Error] defined error of func" << endl;
        exit(0);
    }
    vector<llvm::Value *> VAL_PARA;
    auto ITERARGFUNC = OPWRITFUNC->arg_begin();
    for (tree::Exp *arg : this->args) {
        llvm::Value *funcArgValue = static_cast<llvm::Value *>(ITERARGFUNC++);
        if (funcArgValue->getType()->isPointerTy()) {  // if POINTER
            if (arg->nodeType == ND_VARIABLE_EXP) {    // if vary
                llvm::Value *POINTER = context->getValue(static_cast<tree::VariableExp *>(arg)->name);  // the value of type
                while (POINTER->getType() != llvm::Type::getInt32PtrTy(globalContext)) {
                    POINTER = new llvm::LoadInst(POINTER, llvm::Twine(""), false, context->getCurBasicBlk());
                }
                VAL_PARA.push_back(POINTER);
            } else if (arg->nodeType == ND_BINARY_EXP) {
                BinaryExp *ASTNODE = static_cast<BinaryExp *>(arg);
                switch (ASTNODE->opcode)
                {
                case OP_DOT:
                {
                    if (ASTNODE->OPRSECOND->nodeType == ND_VARIABLE_EXP) {
                        VariableExp *OPRONDSECOND = static_cast<VariableExp *>(ASTNODE->OPRSECOND);
                        int INDEXOFREC        = CodeGenContext::getRecordIndex(ASTNODE->OPRFIRST->returnType, OPRONDSECOND->name);
                        vector<llvm::Value *> INDEXOFA(2);
                        INDEXOFA[0] = llvm::ConstantInt::get( globalContext, llvm::APInt(32, 0, true));
                        INDEXOFA[1] = llvm::ConstantInt::get( globalContext, llvm::APInt(32, INDEXOFREC, true));
                        llvm::Value *POINTER = llvm::GetElementPtrInst::Create(context->getLlvmType(ASTNODE->OPRFIRST->returnType), context->getValue(OPRONDSECOND->name), INDEXOFA, llvm::Twine(""), context->getCurBasicBlk());
                        VAL_PARA.push_back(POINTER);

                    } else {
                        cout << "[Error] OPRSECOND of dot operation is not a variable exp type" << endl;
                        exit(0);
                    }
                    break;
                }
                case OP_INDEX:
                {
                    if (ASTNODE->OPRFIRST->nodeType == ND_VARIABLE_EXP) {
                        VariableExp *operandFIRST = static_cast<VariableExp *>(ASTNODE->OPRFIRST);
                        vector<llvm::Value *> INDEXOFA(2);
                        INDEXOFA[0]        = llvm::ConstantInt::get(globalContext, llvm::APInt(32, 0, true));
                        INDEXOFA[1]        = ASTNODE->OPRSECOND->codeGen(context);
                        llvm::Value *POINTER = llvm::GetElementPtrInst::CreateInBounds(context->getValue(operandFIRST->name),
                            llvm::ArrayRef<llvm::Value *>(INDEXOFA), llvm::Twine("tempname"), context->getCurBasicBlk());
                        VAL_PARA.push_back(POINTER);
                    } else {
                        cout << "[Error] Array's Ref is not an array type variable" << endl;
                        exit(0);
                    }
                    break;
                }
                default:
                    break;
                }
                
            } else {
                cout << "[Error] left value type error " << endl;
            }
        } else {
            VAL_PARA.push_back(arg->codeGen(context));
        }
    }
    cout << "[Success] Function called." << endl;
    return llvm::CallInst::Create(OPWRITFUNC, llvm::makeArrayRef(VAL_PARA), llvm::Twine(""), context->getCurBasicBlk());
}

llvm::Value *tree::LabelStm::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::IfStm::codeGen(CodeGenContext *context) {
    cout << "building while statement" << endl;
    llvm::Value *TCODIN = condition->codeGen(context);
    llvm::BasicBlock *BLK_YES  = llvm::BasicBlock::Create(globalContext, llvm::Twine("then"), context->getCurFunc());
    llvm::BasicBlock *BLK_NO = llvm::BasicBlock::Create(globalContext, llvm::Twine("else"), context->getCurFunc());
    llvm::BasicBlock *MERGEBLK = llvm::BasicBlock::Create(globalContext, llvm::Twine("merge"), context->getCurFunc());
    cout << ">> [Success] Condition generated" << endl;

    llvm::Value *RYUETE = llvm::BranchInst::Create(BLK_YES, BLK_NO,  TCODIN,  context->getCurBasicBlk());          //bsaci of retievce

    context->pushBlock(BLK_YES);
    this->trueBody->codeGen(context);
    cout << ">> [Success] generated True" << endl;
    llvm::BranchInst::Create(  MERGEBLK, context->getCurBasicBlk()); //  merge return while code build true

    context->popBlock();
    context->pushBlock(BLK_NO);
    
    if (this->falseBody != nullptr) {
        this->falseBody->codeGen(context);
        cout << ">> [Success] generated False" << endl;
    }
    llvm::BranchInst::Create( MERGEBLK, context->getCurBasicBlk());   //  merge return while code build false 
    context->popBlock();
    context->pushBlock(MERGEBLK);
    cout << "[Success] If statment generated" << endl;
    return RYUETE;
}

llvm::Value *tree::CaseStm::codeGen(CodeGenContext *context) {
    cout << "Creating case statment" << endl;
    llvm::BasicBlock *ESCBLK = llvm::BasicBlock::Create(globalContext, llvm::Twine("exit"), context->getCurFunc());
    vector<llvm::BasicBlock *> TBLKS;
    cout << ">> Contains :" << situations.size() << "cases" << endl;

    llvm::Value *RETIV;
    int i = 0;
    while( i< this->situations.size())
    {   
        for (int j = 0; j < this->situations[i]->caseVec.size(); j++) {
            llvm::BasicBlock *BLKBASE =
                llvm::BasicBlock::Create(globalContext, llvm::Twine("case"), context->getCurFunc());

            cout << ">> In case " << i << ":" << j << endl;
            tree::BinaryExp *CONDITION = new tree::BinaryExp(OP_EQUAL, this->object, this->situations[i]->caseVec[j]);

            if (i == this->situations.size() - 1 && j == this->situations[i]->caseVec.size() - 1) {  // last one
                RETIV = llvm::BranchInst::Create( BLKBASE, ESCBLK, CONDITION->codeGen(context), context->getCurBasicBlk());  // insert the last one 
            } else {
                llvm::BasicBlock *BLKNEXT = llvm::BasicBlock::Create(globalContext, llvm::Twine("next"), context->getCurFunc());
                llvm::BranchInst::Create(BLKBASE, BLKNEXT,  CONDITION->codeGen(context), context->getCurBasicBlk());     // not true would be next
                context->pushBlock(BLKNEXT);
            }
        }
        i++;
    }

    i = 0;
    while( i< this->situations.size())
    {   
        for (int j = 0; j < this->situations[i]->caseVec.size(); j++) {  // situation with match
            llvm::BasicBlock *BLKBASE = llvm::BasicBlock::Create(globalContext, llvm::Twine("caseStmt"), context->getCurFunc());
            TBLKS.push_back(BLKBASE);  // a block of  push
        }
        i++;
    }

    int p = 0;
    i = 0;

    while( i< this->situations.size())
    {   
        for (int j = 0; j < this->situations[i]->caseVec.size(); j++, p++) {
            cout << "in case No." << i << endl;
            cout << "|__case's No." << j << endl;

            tree::BinaryExp *CONDITION = new tree::BinaryExp(OP_EQUAL, this->object, this->situations[i]->caseVec[j]);

            llvm::BasicBlock *BLKNEXT;
            if (p == TBLKS.size() - 1) {
                RETIV = llvm::BranchInst::Create(TBLKS[p], ESCBLK,  CONDITION->codeGen(context),  context->getCurBasicBlk());            // END insret
            } else {
                BLKNEXT = llvm::BasicBlock::Create(globalContext, "next", context->getCurFunc());

                llvm::BranchInst::Create(TBLKS[p],  BLKNEXT,   CONDITION->codeGen(context),  context->getCurBasicBlk());      //basic create

                context->pushBlock(BLKNEXT);
            }
        }
        i++;
        p++;
    }


    p = 0;
    i = 0;
    while( i< this->situations.size())
    {   
        for (int j = 0; j < this->situations[i]->caseVec.size(); j++, p++) {
            context->pushBlock(TBLKS[p]);
            this->situations[i]->codeGen(context);
            llvm::BranchInst::Create(ESCBLK, context->getCurBasicBlk());
            cout << ">> [Success] In case " << i << ":" << j << endl;
            context->popBlock();
        }
        i++;
        p++;
    }

    context->pushBlock(ESCBLK);

    return RETIV;
}

llvm::Value *tree::ForStm::codeGen(CodeGenContext *context) {
    cout << "Creating for statement" << endl;
    llvm::BasicBlock *BLKSRT = llvm::BasicBlock::Create(globalContext, llvm::Twine("start"), context->getCurFunc());
    llvm::BasicBlock *BLKLOP  = llvm::BasicBlock::Create(globalContext, llvm::Twine("loop"), context->getCurFunc());
    llvm::BasicBlock *BLKLESC  = llvm::BasicBlock::Create(globalContext, llvm::Twine("exit"), context->getCurFunc());
    // 定义循环变量
    tree::VariableExp *PARALOOP   = new tree::VariableExp(this->iter);
    PARALOOP->returnType          = tree::findVar(this->iter, this);
    PARALOOP->name                = this->iter;
    tree::AssignStm *LOOPINITPARA = new tree::AssignStm( PARALOOP, this->start);
    LOOPINITPARA->codeGen(context);  
    llvm::BranchInst::Create(BLKSRT, context->getCurBasicBlk());
 
    context->pushBlock(BLKSRT);                
    tree::BinaryExp *COMPARE   = new tree::BinaryExp(  OP_EQUAL, PARALOOP, this->end);
    llvm::Instruction *RETC = llvm::BranchInst::Create(BLKLESC,  BLKLOP, COMPARE->codeGen(context), context->getCurBasicBlk()); //to the end or not 
    context->popBlock();
    context->pushBlock(BLKLOP);  
    this->loop->codeGen(context);   

    // LOOPUPDATE
    tree::BinaryExp *LOOPUPDATE;
    tree::Value *TMP               = new tree::Value();
    TMP->baseType                  = TY_INT;
    TMP->val.intVal                = this->step;  
    tree::ConstantExp *int1        = new tree::ConstantExp(TMP);
    LOOPUPDATE                         = new tree::BinaryExp(  OP_ADD, PARALOOP, int1);
    tree::AssignStm *updateLoopVar = new tree::AssignStm( PARALOOP, LOOPUPDATE);
    updateLoopVar->codeGen(context);  
    llvm::BranchInst::Create( BLKSRT, context->getCurBasicBlk());
    context->popBlock();
    context->pushBlock(BLKLESC);  
    this->loop->codeGen(context);   
    cout << "[Success] For loop generated" << endl;
    return RETC;
}

llvm::Value *tree::WhileStm::codeGen(CodeGenContext *context) {
    cout << "Building WIHILE statement" << endl;
    llvm::BasicBlock *BLKST = llvm::BasicBlock::Create(globalContext, llvm::Twine("start"), context->getCurFunc());
    llvm::BasicBlock *BLKLOP  = llvm::BasicBlock::Create(globalContext, llvm::Twine("loop"), context->getCurFunc());
    llvm::BasicBlock *BLKESC  = llvm::BasicBlock::Create(globalContext, llvm::Twine("exit"), context->getCurFunc());

    llvm::BranchInst::Create( BLKST, context->getCurBasicBlk());
    context->pushBlock(BLKST);                         // give the start
    llvm::Value *RECT = llvm::BranchInst::Create(BLKLOP,  BLKESC, this->condition->codeGen(context), context->getCurBasicBlk()); // true jump or nit
    context->popBlock();
    context->pushBlock(BLKLOP);  
    this->loop->codeGen(context);   // still cong
    llvm::BranchInst::Create( BLKST, context->getCurBasicBlk());
    context->popBlock();
    context->pushBlock(BLKESC);  // out boclk
    cout << "[Success] While loop create" << endl;
    return RECT;
}

llvm::Value *tree::RepeatStm::codeGen(CodeGenContext *context) {
    cout << "Building repeat statement" << endl;
    llvm::BasicBlock *BLKLOP = llvm::BasicBlock::Create(globalContext, llvm::Twine("loop"), context->getCurFunc());
    llvm::BranchInst::Create( BLKLOP, context->getCurBasicBlk());
    context->pushBlock(BLKLOP);                                // loop
    this->loop->codeGen(context);                                 // build blk

    llvm::Value *CONDITION      = this->condition->codeGen(context);   // cond
    
    llvm::BasicBlock *BLKESC = llvm::BasicBlock::Create(globalContext, llvm::Twine("exit"), context->getCurFunc());

    llvm::Instruction *RECT = llvm::BranchInst::Create(BLKESC,  BLKLOP, CONDITION, context->getCurBasicBlk()); //true jump or not
    context->popBlock();
    context->pushBlock(BLKESC);  // BLKLESC
    cout << "[Success] Repeat loop created" << endl;
    return RECT;
}

llvm::Value *tree::GotoStm::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::UnaryExp::codeGen(CodeGenContext *context) {
    if (this->opcode == OP_NOT)
    {
        return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT,this->operand->codeGen(context), llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true), llvm::Twine(""), context->getCurBasicBlk());
    }
    else if(this->opcode == OP_OPPO)
    {
        if (this->operand->returnType->baseType == TY_INT) {  // define n= 0- n
                return llvm::BinaryOperator::Create(llvm::Instruction::Sub,llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true),this->operand->codeGen(context), llvm::Twine(""), context->getCurBasicBlk());
            } 
            else if (this->operand->returnType->baseType == TY_REAL) {
                return llvm::BinaryOperator::Create(llvm::Instruction::FSub,llvm::ConstantFP::get(globalContext, llvm::APFloat(0.)), this->operand->codeGen(context),llvm::Twine(""), context->getCurBasicBlk());
            }
    }
    else if(this->opcode == OP_PRED)
    {
        return llvm::BinaryOperator::Create(  llvm::Instruction::Sub, this->operand->codeGen(context),llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), 1, true), llvm::Twine(""),context->getCurBasicBlk());
    }
    else if(this->opcode == OP_SUCC)
    {
        return llvm::BinaryOperator::Create( llvm::Instruction::Add, this->operand->codeGen(context),llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), 1, true), llvm::Twine(""),context->getCurBasicBlk());
    }
    else if(this->opcode == OP_ODD)
    {   //0 & n
        return llvm::BinaryOperator::Create( llvm::Instruction::And, llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true),this->operand->codeGen(context), llvm::Twine(""), context->getCurBasicBlk());
    }
    else if(this->opcode == OP_ORD)
    {
        return llvm::CastInst::CreateIntegerCast(this->operand->codeGen(context),llvm::Type::getInt32Ty(globalContext), true, llvm::Twine(""), context->getCurBasicBlk());

    }
    else if(this->opcode == OP_CHR)
    {
        return llvm::CastInst::CreateIntegerCast(this->operand->codeGen(context),llvm::Type::getInt8Ty(globalContext), true, llvm::Twine(""), context->getCurBasicBlk());

    }
    else if(this->opcode == OP_ABS)
    {
        if (this->operand->returnType->baseType == TY_INT) {
            llvm::Value *CONDITION = llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT,llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true),this->operand->codeGen(context), llvm::Twine(""), context->getCurBasicBlk());
            llvm::BasicBlock *BLKDOWN = llvm::BasicBlock::Create(globalContext, llvm::Twine("neg"), context->getCurFunc());
            llvm::BasicBlock *BLKUP = llvm::BasicBlock::Create(globalContext, llvm::Twine("pos"), context->getCurFunc());
            llvm::BasicBlock *BLKMERGE = llvm::BasicBlock::Create(globalContext, llvm::Twine("merge"), context->getCurFunc());
            llvm::Value *RETC = llvm::BranchInst::Create(BLKDOWN, BLKUP,  CONDITION, context->getCurBasicBlk()); //positve or negative

            context->pushBlock(BLKDOWN);  
            llvm::BinaryOperator::Create(llvm::Instruction::Sub, llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), 0, true),this->operand->codeGen(context), llvm::Twine(""), context->getCurBasicBlk());
            llvm::BranchInst::Create( BLKMERGE, context->getCurBasicBlk());  // jump BLKMERGE
                    
            context->popBlock();
            context->pushBlock(BLKUP);     // not true
            this->operand->codeGen(context);  // abs
            llvm::BranchInst::Create(  BLKMERGE, context->getCurBasicBlk());    // jump BLKMERGE
            context->popBlock();
            context->pushBlock(BLKMERGE);  // BLKMERGE
            return RETC;
        }
        else if (this->operand->returnType->baseType == TY_REAL) {
            llvm::Value *CONDITION = llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT,llvm::ConstantFP::get(globalContext, llvm::APFloat(0.)), this->operand->codeGen(context),llvm::Twine(""), context->getCurBasicBlk());
            llvm::BasicBlock *BLKDOWN = llvm::BasicBlock::Create(globalContext, llvm::Twine("neg"), context->getCurFunc());
            llvm::BasicBlock *BLKUP = llvm::BasicBlock::Create(globalContext, llvm::Twine("pos"), context->getCurFunc());
            llvm::BasicBlock *BLKMERGE = llvm::BasicBlock::Create(globalContext, llvm::Twine("merge"), context->getCurFunc());
            llvm::Value *RETC = llvm::BranchInst::Create(BLKDOWN, BLKUP, CONDITION, context->getCurBasicBlk());
            context->pushBlock(BLKDOWN);
            
            llvm::BinaryOperator::Create(llvm::Instruction::FSub, llvm::ConstantFP::get(globalContext, llvm::APFloat(0.)), this->operand->codeGen(context), llvm::Twine(""), context->getCurBasicBlk());
            llvm::BranchInst::Create(BLKMERGE, context->getCurBasicBlk());
            context->popBlock();
            context->pushBlock(BLKUP);
            this->operand->codeGen(context);
            llvm::BranchInst::Create(BLKMERGE, context->getCurBasicBlk());
            context->popBlock();
            context->pushBlock(BLKMERGE);
            return RETC;
        }
    }
    return nullptr;
}

llvm::Value *tree::BinaryExp::codeGen(CodeGenContext *context) {

    switch (this->opcode)
    {
    case OP_DOT:{
       if (this->OPRSECOND->nodeType == ND_VARIABLE_EXP) {  // vary of op2
            tree::VariableExp *OPRONDSECOND = static_cast<tree::VariableExp *>(this->OPRSECOND);
            int INDEXOFREC              = CodeGenContext::getRecordIndex(this->OPRFIRST->returnType, OPRONDSECOND->name); //find pos
            vector<llvm::Value *> INDEXOFA(2);
            INDEXOFA[0]           = llvm::ConstantInt::get(  // 0
                globalContext, llvm::APInt(32, 0, true));
            INDEXOFA[1]           = llvm::ConstantInt::get(                // index
                globalContext, llvm::APInt(32, INDEXOFREC, true));  // create member_index
            llvm::Value *memPtr = llvm::GetElementPtrInst::Create(context->getLlvmType(this->OPRFIRST->returnType), context->getValue(OPRONDSECOND->name), INDEXOFA, llvm::Twine(""), context->getCurBasicBlk());
            return new llvm::LoadInst(memPtr, llvm::Twine(""), false, context->getCurBasicBlk());
        } else {
            cout << "[Error] error member of record " << endl;
            exit(0);
        }        
        break;
    }

    case OP_INDEX:{
        if (this->OPRFIRST->nodeType == ND_VARIABLE_EXP) {
            tree::VariableExp *OPRONDFIRST = static_cast<tree::VariableExp *>(this->OPRFIRST);
            vector<llvm::Value *> INDEXOFA(2);
            INDEXOFA[0]           = llvm::ConstantInt::get(globalContext, llvm::APInt(32, 0, true));
            INDEXOFA[1]           = this->OPRSECOND->codeGen(context);
            llvm::Value *memPtr = llvm::GetElementPtrInst::CreateInBounds(context->getValue(OPRONDFIRST->name), llvm::ArrayRef<llvm::Value *>(INDEXOFA), llvm::Twine("tempname"), context->getCurBasicBlk());
            return new llvm::LoadInst(memPtr, llvm::Twine(""), false, context->getCurBasicBlk());
        } else {
            cout << "[Error] List's reference not true with the type" << endl;
            exit(0);
        }        
        break;
    }
    
    default:
        break;
    }

    llvm::Value *VAL_OPREANDFIRST_ = this->OPRFIRST->codeGen(context);
    llvm::Value *VAL_OPREANDSECOND_ = this->OPRSECOND->codeGen(context);

    if (VAL_OPREANDFIRST_->getType()->isFloatTy() || VAL_OPREANDSECOND_->getType()->isFloatTy()) {  // if float
        switch (this->opcode) {
            case OP_ADD:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::FAdd, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MINUS:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::FSub, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MULTI:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::FMul, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_RDIV:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::FDiv, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_DDIV:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::SDiv, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MOD:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::SRem, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_AND:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::And, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_OR:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Or, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            // logical
            case OP_SMALL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLT, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_LARGE:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_SMALL_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGE, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_LARGE_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLE, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_EQ, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_NOT_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_NE, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            default:
                cout << "[Error] error type of opreation:" << opcode << endl;  
                exit(0);
        }
    } else {  // int 
        switch (opcode) {
            case OP_ADD:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Add, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MINUS:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Sub, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MULTI:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Mul, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_RDIV:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::UDiv, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_DDIV:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::SDiv, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_MOD:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::SRem, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_AND:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::And, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_OR:
                return llvm::BinaryOperator::Create(
                    llvm::Instruction::Or, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_, llvm::Twine(""), context->getCurBasicBlk());
            case OP_SMALL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLT, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_LARGE:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGT, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_SMALL_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SGE, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_LARGE_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_SLE, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_EQ, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            case OP_NOT_EQUAL:
                return llvm::CmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::ICMP_NE, VAL_OPREANDFIRST_, VAL_OPREANDSECOND_,
                    llvm::Twine(""), context->getCurBasicBlk());
            default: cout << "[Error] Unknown type of opcode:" << opcode << endl; exit(0);
        }
    }
}

llvm::Value *tree::CallExp::codeGen(CodeGenContext *context) {
    cout << "Building calling " << endl;
    llvm::Function *FUcTION = context->pModule->getFunction(this->name.c_str());
    if (FUcTION == nullptr) {
        cout << "[Error] Function: " << name << " didn't defined" << endl;
        exit(0);
    }

    vector<llvm::Value *> VALUEPARA;
    auto ITERFUNPARA = FUcTION->arg_begin();
    for (tree::Exp *arg : this->args) {
        llvm::Value *funcArgValue = static_cast<llvm::Value *>(ITERFUNPARA++);
        if (funcArgValue->getType()->isPointerTy()) {  // if ptr
            if (arg->nodeType == ND_VARIABLE_EXP) {    // if vary
                llvm::Value *POINTER = context->getValue(static_cast<tree::VariableExp *>(arg)->name);  // value
                while (POINTER->getType() != llvm::Type::getInt32PtrTy(globalContext)) {
                    POINTER = new llvm::LoadInst(POINTER, llvm::Twine(""), false, context->getCurBasicBlk());
                }
                VALUEPARA.push_back(POINTER);
            } 
            else if (arg->nodeType == ND_BINARY_EXP) {
                BinaryExp *ASTNODE = static_cast<BinaryExp *>(arg);

                switch (ASTNODE->opcode)
                {
                case OP_DOT:{
                    if (ASTNODE->OPRSECOND->nodeType == ND_VARIABLE_EXP) {
                        VariableExp *OPRONDSECOND = static_cast<VariableExp *>(ASTNODE->OPRSECOND);
                        int INDEXOFREC        = CodeGenContext::getRecordIndex(ASTNODE->OPRFIRST->returnType, OPRONDSECOND->name);
                        vector<llvm::Value *> INDEXOFA(2);
                        INDEXOFA[0] = llvm::ConstantInt::get( globalContext, llvm::APInt(32, 0, true)); //zero
                        INDEXOFA[1] = llvm::ConstantInt::get(  globalContext, llvm::APInt(32, INDEXOFREC, true));  //index
                        llvm::Value *POINTER =
                            llvm::GetElementPtrInst::Create(context->getLlvmType(ASTNODE->OPRFIRST->returnType), context->getValue(OPRONDSECOND->name), INDEXOFA,  llvm::Twine(""), context->getCurBasicBlk()); //record the name 
                        VALUEPARA.push_back(POINTER);
                    } else {
                        cout << "[Error] OPRSECOND of dot operation would not match the exp type" << endl;
                        exit(0);
                    }                    
                    break;
                }
                
                case OP_INDEX:{
                    if (ASTNODE->OPRFIRST->nodeType == ND_VARIABLE_EXP) {
                        VariableExp *OPRONDFIRST = static_cast<VariableExp *>(ASTNODE->OPRFIRST);
                        vector<llvm::Value *> INDEXOFA(2);
                        INDEXOFA[0]        = llvm::ConstantInt::get(globalContext, llvm::APInt(32, 0, true));
                        INDEXOFA[1]        = ASTNODE->OPRSECOND->codeGen(context);
                        llvm::Value *POINTER = llvm::GetElementPtrInst::CreateInBounds(context->getValue(OPRONDFIRST->name),
                            llvm::ArrayRef<llvm::Value *>(INDEXOFA), llvm::Twine("tempname"), context->getCurBasicBlk());
                        VALUEPARA.push_back(POINTER);
                    } else {
                        cout << "[Error] List's reference not true with the type" << endl;
                        exit(0);
                    }                
                }

                default:
                    break;
                }
            
            } else {
                cout << "[Error] left value type error" << endl;
            }
        } else {
            VALUEPARA.push_back(arg->codeGen(context));
        }
    }
    return llvm::CallInst::Create(FUcTION, llvm::makeArrayRef(VALUEPARA), llvm::Twine(""), context->getCurBasicBlk());
}

llvm::Value *tree::ConstantExp::codeGen(CodeGenContext *context) {
    return value->codeGen(context);
}

llvm::Value *tree::VariableExp::codeGen(CodeGenContext *context) {
    cout << "variable load with: '" << this->name << "'" << endl;
    llvm::Value *PTRIONT = context->getValue(this->name);
    PTRIONT = new llvm::LoadInst(PTRIONT, llvm::Twine(""), false, context->getCurBasicBlk());
    if (PTRIONT->getType()->isPointerTy()) {
        PTRIONT = new llvm::LoadInst(PTRIONT, llvm::Twine(""), false, context->getCurBasicBlk());
    }
    return PTRIONT;
}

llvm::Value *tree::Type::codeGen(CodeGenContext *context) {
    return nullptr;
}

llvm::Value *tree::Value::codeGen(CodeGenContext *context) {

    if (this->baseType == TY_INT)
    {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(globalContext), this->val.intVal, true);
    }
    else if(this->baseType == TY_REAL)
    {
         return llvm::ConstantFP::get(globalContext, llvm::APFloat(this->val.realVal));
    }
    else if(this->baseType == TY_CHAR)
    {
        return llvm::ConstantInt::get(llvm::Type::getInt8Ty(globalContext), this->val.charVal, true);
    }
    else if(this->baseType == TY_BOOL)
    {
        return llvm::ConstantInt::get(llvm::Type::getInt1Ty(globalContext), this->val.boolVal, true);
    }
    else{
        return nullptr;
    }
    
}
