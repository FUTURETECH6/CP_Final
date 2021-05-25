.PHONY: all clean scan test fmt progs dirs clean_tmp

TARGET = MiniPascal

# Config Env
LLVM_DIR = /usr/lib/llvm-8/
CXX = g++

# Binaries
LLVM_CONFIG = ${LLVM_DIR}/bin/llvm-config
LLVM_CLANG  = ${LLVM_DIR}/bin/clang
LLVM_OPT    = ${LLVM_DIR}/bin/opt
LLVM_DIS    = ${LLVM_DIR}/bin/llvm-dis
LLVM_LLI    = ${LLVM_DIR}/bin/lli
LLVM_LLC    = ${LLVM_DIR}/bin/llc
LLVM_FMT    = ${LLVM_DIR}/bin/clang-format

# Flags
INCLUDE = -Iinclude -Ibuild
LLVM_LIBS = $(shell $(LLVM_CONFIG) --ldflags --libs)

CCFLAGS = $(shell $(LLVM_CONFIG) --cppflags) -g -std=c++11 ${INCLUDE}
LDFLAGS = ${CCFLAGS} ${LLVM_LIBS} -ll

all: dirs progs

progs: ${TARGET}
${TARGET}: build/parser.o build/lexer.o build/tree.o build/symbol.o build/semantics.o build/codegen.o build/main.o
	$(CXX) $^ ${LDFLAGS} -o $@

build/parser.o: build/parser.cpp
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/lexer.o: build/lexer.cpp build/parser.cpp
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/tree.o: src/tree.cpp include/tree.h
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/symbol.o: src/symbol.cpp include/symbol.h
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/semantics.o: src/semantics.cpp include/tree.h include/symbol.h
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/codegen.o: src/codegen.cpp include/codegen.h
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/main.o: src/main.cpp include/tree.h include/codegen.h build/parser.cpp
	$(CXX) -o $@ -c $< ${CCFLAGS}

build/lexer.cpp: src/lexer.l 
	flex -o $@ $<
build/parser.cpp: src/parser.y 
	bison -o $@ -d $<

dirs: build
build:
	mkdir -p build

clean: clean_tmp
	@rm -f ${TARGET}
clean_tmp:
	@rm -rf build
	@rm -f bitcode.bin* a.tree
	@rm -f test/*.ll test/*.lli test/*.s test/*.tree


scan:
	intercept-build make && analyze-build

test: all 
	for i in test1 test2 test3 test4 test5 test6 test7 ; do \
		./$(TARGET) test/$$i.pas ; \
		$(LLVM_DIS) bitcode.bin ;  \
		$(LLVM_LLC) bitcode.bin.ll -march=x86-64 ; \
		mv a.tree test/$$i.tree ; \
		mv bitcode.bin.ll test/$$i.ll ; \
		mv bitcode.bin.s test/$$i.s ; \
	done

fmt:
	$(LLVM_FMT) -i --style=file src/**.cpp include/*.h
	ls test/*.pas | xargs -i ptop -c ptop.cfg {} {}