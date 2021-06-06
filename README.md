# Prerequisites

For building:

```bash
sudo apt install -y llvm-8 llvm-8-dev llvm-8-tools build-essential
sudo apt install -y clang-8  # Unnecessary if you use gnu-g++ as CXX
```

For style formatting:

```bash
sudo apt install -y clang-format-8 fpc
```

# Usage

Build only:

```bash
make
```

Test one case:

```bash
./PasGo <path/to/pascal/file>
```

Build and test all cases:

```bash
make test
```

Build and test and print all test output:

```bash
make print_output
```

## What will the program PasGo do?

1. generate AST at "./a.tree"
2. generate bitcode at "./a.bc"
3. run the code

## What will the `make test` do?

1. put the stdout of program at "tgt/\$CASE/\$CASE(debug output).txt"
2. put tree and bc at ""tgt/\$CASE/"
3. run bitcode using llvm interpreter and put the output at "tgt/\$CASE/\$CASE(lli output).txt"
4. generate IR file using llvm disassembler at "tgt/\$CASE/\$CASE.ll"
5. generate assembly file using llvm compiler at "tgt/\$CASE/\$CASE.s"(using x86_64 here, but can be specified in Makefile)