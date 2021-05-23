.PHONY: all clean scan progs dirs

NAME = Tiny-Pascal

# Config Env
LLVM_CONFIG = llvm-config-8
CXX = g++

INCLUDE = -Iinclude -Ibuild
LLVM_LIBS = $(shell $(LLVM_CONFIG) --ldflags --libs)

CCFLAGS = $(shell $(LLVM_CONFIG) --cppflags) -g -std=c++11 ${INCLUDE}
LDFLAGS = ${CCFLAGS} ${LLVM_LIBS} -ll

all: dirs progs

progs: Tiny-Pascal
Tiny-Pascal: build/ast.o build/codegen.o build/main.o build/semantics.o build/util.o build/tokenizer.o build/parser.o
	$(CXX) $^ ${LDFLAGS} -o $@

build/ast.o: src/ast.cpp include/ast.h
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/codegen.o: src/codegen.cpp include/codegen.h
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/main.o: src/main.cpp include/ast.h include/codegen.h build/parser.cpp
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/semantics.o: src/semantics.cpp include/ast.h include/util.h
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/util.o: src/util.cpp include/util.h
	$(CXX) -o $@ -c $< ${CCFLAGS}

build/tokenizer.o: build/tokenizer.cpp build/parser.cpp
	$(CXX) -o $@ -c $< ${CCFLAGS}
build/parser.o: build/parser.cpp
	$(CXX) -o $@ -c $< ${CCFLAGS}

build/tokenizer.cpp: src/${NAME}.l 
	flex -o $@ $<
build/parser.cpp: src/${NAME}.y 
	bison -o $@ -d $<

dirs: build
build:
	mkdir -p build

clean:
	@rm -rf build
	@rm -f Tiny-Pascal
	@rm -f LLVM_IR print.t

scan:
	intercept-build make && analyze-build
