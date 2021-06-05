.PHONY: all clean scan test fmt progs dirs clean_tmp clean_test print_output

TARGET = PasGo

# Config Env
LLVM_DIR = /usr/lib/llvm-8

# Binaries
LLVM_CONFIG = ${LLVM_DIR}/bin/llvm-config
LLVM_CLANG  = ${LLVM_DIR}/bin/clang
LLVM_OPT    = ${LLVM_DIR}/bin/opt
LLVM_DIS    = ${LLVM_DIR}/bin/llvm-dis
LLVM_LLI    = ${LLVM_DIR}/bin/lli
LLVM_LLC    = ${LLVM_DIR}/bin/llc
LLVM_FMT    = ${LLVM_DIR}/bin/clang-format

# CXX = g++
CXX = $(LLVM_CLANG)

# Flags
INCLUDE = -Iinclude -Ibuild
LLVM_LIBS = $(shell $(LLVM_CONFIG) --ldflags --libs)

CCFLAGS = $(shell $(LLVM_CONFIG) --cppflags)  -std=c++11 ${INCLUDE} -Ofast
LDFLAGS = ${CCFLAGS} ${LLVM_LIBS} -ll -lstdc++

all: dirs progs

progs: ${TARGET}
${TARGET}: build/parser.o build/lexer.o build/tree.o build/treeprint.o build/symbol.o build/semantics.o build/codegen.o build/main.o
	$(CXX) $^ ${LDFLAGS} -o $@

build/parser.o: build/parser.cpp
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/lexer.o: build/lexer.cpp build/parser.cpp
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/tree.o: src/tree.cpp include/tree.h
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/treeprint.o: src/treeprint.cpp include/tree.h
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

clean: clean_tmp clean_test
	@rm -f ${TARGET}
clean_tmp:
	@rm -rf build
clean_test:
	@rm -f a.bc* a.tree
	@rm -f test/*.bc test/*.ll test/*.lli test/*.s test/*.tree test/*.txt
	@rm -rf tgt

scan:
	intercept-build make && analyze-build

tgt_dir=tgt/$$i/
test: all
	for i in $(notdir $(patsubst %.pas, %, $(wildcard test/*.pas))) ; do \
		echo $$i ; \
		mkdir -p ${tgt_dir} ; \
		./$(TARGET) test/$$i.pas > "${tgt_dir}/$$i(debug output).txt" ; \
		mv a.bc ${tgt_dir}/$$i.bc ; \
		mv a.tree ${tgt_dir}/$$i.tree ; \
		$(LLVM_LLI) ${tgt_dir}/$$i.bc > "${tgt_dir}/$$i(lli output).txt" ;  \
		$(LLVM_DIS) -o ${tgt_dir}/$$i.ll ${tgt_dir}/$$i.bc ;  \
		$(LLVM_LLC) -o ${tgt_dir}/$$i.s ${tgt_dir}/$$i.ll -march=x86-64 ; \
	done

fmt:
	$(LLVM_FMT) -i --style=file src/**.cpp include/*.h
	ls test/*.pas | xargs -i ptop -i 2 -c ptop.cfg {} {}

print_output: fmt clean test
	ls ${tgt_dir} | xargs -i sh -c 'echo "\n\n{}:" &&  cat "${tgt_dir}/{}/{}(lli output).txt"'
