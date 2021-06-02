#include <iostream>

#include "codegen.h"
#include "parser.hpp"
#include "tree.h"

extern int doyyparse(char *file);
extern tree::Program *treeRoot;

int main(int argc, char **argv) {
    if (doyyparse(argv[1])) {
        throw std::runtime_error("No such file or directory");
        return -1;
    }
    std::cout << "after yyparse()" << std::endl;
    tree::printTree("a.tree", treeRoot);

    if (treeRoot->checkSemantics() == false) {
        return 0;
    }
    std::cout << "semantics passed" << std::endl;
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTarget();
    CodeGenContext context;

    context.generateCode(*treeRoot, "a.bc");

    context.runCode();
    return 0;
}
