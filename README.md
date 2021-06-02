# Prerequisite

For building:

```bash
sudo apt install -y llvm-8 llvm-8-dev llvm-8-tools build-essential
sudo apt install -y clang-8  # Unnecessary if you use gnu-g++ as CXX
```

For formatting:

```bash
sudo apt install -y clang-format-8 fpc
```

# Usage

Build only:

```bash
make
```

Test:

```bash
./MiniPascal <path/to/pascal/file>
```

Build and test:

```bash
make test
```

Remove temporary files:

```bash
make clean_tmp
```