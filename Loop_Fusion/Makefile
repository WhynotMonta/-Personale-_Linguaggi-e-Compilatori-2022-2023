OPTIMIZER := lib/LoopFusion.so
OBJs := $(subst .cpp,.o,$(wildcard lib/*.cpp))

LLVM_VERSION ?= 14

CXXFLAGS := $(shell llvm-config --cxxflags) -fPIC
LLVMFLAGS := $(shell llvm-config --ldflags --libs --system-libs)

all: $(OPTIMIZER) 

$(OPTIMIZER): $(OBJs)
	$(CXX) ${LLVMFLAGS} -dylib -shared $^ -o $@

.PHONY: clean
clean:
	$(RM) $(OPTIMIZER) $(OBJs)

compile:
	clang -O0 -Xclang -disable-O0-optnone -emit-llvm -c ./test/Loop.c -o ./test/Loop.bc
	opt -passes=mem2reg ./test/Loop.bc -o ./test/Loop.opt.bc
	llvm-dis ./test/Loop.opt.bc -o ./test/Loop.opt.ll

load:
	opt -load-pass-plugin=./lib/LoopFusion.so -passes=loop_fusion ./test/Loop.opt.ll -o Loop.bc
	llvm-dis Loop.bc -o Loop.ll