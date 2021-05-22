.PHONY: all clean scan progs dirs

NAME = Tiny-Pascal

LLVM_LIBS = $(shell llvm-config --ldflags --libs)
INCLUDE = -Iinclude -Ibuild

CXXFLAGS = $(shell llvm-config --cppflags) -g -std=c++11 ${INCLUDE}
LDFLAGS  = ${CXXFLAGS} ${LLVM_LIBS} -ll

CXX = c++
LD  = c++

DEPENDECIES = $(wildcard include/*.h) Makefile

all: dirs progs

progs: Tiny-Pascal
Tiny-Pascal: build/ast.o build/codegen.o build/semantics.o build/util.o build/tokenizer.o build/parser.o src/main.cpp
	$(LD) $^ ${LDFLAGS} -o $@

build/ast.o: src/ast.cpp include/ast.h
	$(CXX) -o $@ -c $< ${CXXFLAGS}
build/codegen.o: src/codegen.cpp include/codegen.h
	$(CXX) -o $@ -c $< ${CXXFLAGS}
build/semantics.o: src/semantics.cpp include/ast.h include/util.h
	$(CXX) -o $@ -c $< ${CXXFLAGS}
build/util.o: src/util.cpp include/util.h
	$(CXX) -o $@ -c $< ${CXXFLAGS}

build/tokenizer.o: build/tokenizer.cpp build/parser.cpp
	$(CXX) -o $@ -c $< ${CXXFLAGS}
build/parser.o: build/parser.cpp
	$(CXX) -o $@ -c $< ${CXXFLAGS}

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
