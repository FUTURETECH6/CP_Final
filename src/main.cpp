#include <iostream>

#include "codegen.h"
#include "parser.hpp"
#include "tree.h"

extern int doyyparse(char *file);
extern tree::Program *ast_root;

int main(int argc, char **argv) {
    if (doyyparse(argv[1])) {
        throw std::runtime_error("No such file or directory");
        return -1;
    }
    std::cout << "after yyparse()" << std::endl;
    tree::printTree("a.tree", ast_root);

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
