#include <iostream>

#include "codegen.h"
#include "parser.hpp"
#include "tree.h"

extern int doyyparse(char *file);
extern tree::Program *treeRoot;

int main(int argc, char **argv) {
    using namespace std;
    if (doyyparse(argv[1])) {
        throw runtime_error("We not find such file or directory");
        return -1;
    }
    cout << "after yyparse()" << endl;
    tree::visualizeTree("a.tree", treeRoot);

    if (treeRoot->SEMANT_CHECK_LEGAL() == false) {
        return 0;
    }
    cout << "semantics succeed" << endl;
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTarget();
    ContextOfCodeCreate context;

    context.CODEGENER(*treeRoot, "a.bc");

    context.runCode();
    return 0;
}
