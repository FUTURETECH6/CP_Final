.PHONY: all clean scan progs dirs

BINARY = MiniPascal

# Config Env
LLVM_CONFIG = llvm-config-8
CXX = g++

INCLUDE = -Iinclude -Ibuild
LLVM_LIBS = $(shell $(LLVM_CONFIG) --ldflags --libs)

CCFLAGS = $(shell $(LLVM_CONFIG) --cppflags) -g -std=c++11 ${INCLUDE}
LDFLAGS = ${CCFLAGS} ${LLVM_LIBS} -ll

all: dirs progs

progs: ${BINARY}
${BINARY}: build/parser.o build/lexer.o build/tree.o build/symbol.o build/semantics.o build/codegen.o build/main.o
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

clean:
	@rm -rf build
	@rm -f ${BINARY}
	@rm -f LLVM_IR print.t

scan:
	intercept-build make && analyze-build
