#include <iostream>

#include "codegen.h"
#include "parser.hpp"
#include "tree.h"

extern int doyyparse(char *file);
extern tree::Program *treeRoot;

int main(int argc, char **argv) {
    using namespace std;
    if (doyyparse(argv[1])) {
        throw runtime_error("No such file or directory");
        return -1;
    }
    cout << "after yyparse()" << endl;
    tree::visualizeTree("a.tree", treeRoot);

    if (treeRoot->checkSemantics() == false) {
        return 0;
    }
    cout << "semantics passed" << endl;
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTarget();
    CodeGenContext context;

    context.generateCode(*treeRoot, "a.bc");

    context.runCode();
    return 0;
}
