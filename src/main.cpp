#include <iostream>

#include "ast.h"
#include "codegen.h"
#include "parser.hpp"

extern int doyyparse(char *file);
extern ast::Program *ast_root;

int main(int argc, char **argv) {
    doyyparse(argv[1]);
    std::cout << "after yyparse()" << std::endl;
    ast::printTree("print", ast_root);

    if (ast_root->checkSemantics() == false) {
        return 0;
    }
    std::cout << "semantics passed" << std::endl;
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTarget();
    CodeGenContext context;

    context.generateCode(*ast_root);

    context.runCode();
    return 0;
}
