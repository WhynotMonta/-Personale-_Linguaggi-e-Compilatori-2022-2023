OPTIMIZER := lib/LoopWalk.so
OBJs := $(subst .cpp,.o,$(wildcard lib/*.cpp))

CXXFLAGS := $(shell llvm-config --cxxflags) -fPIC 
LLVMFLAGS := $(shell llvm-config --ldflags --libs --system-libs)


all: $(OPTIMIZER) 

$(OPTIMIZER): $(OBJs)
	$(CXX) ${LLVMFLAGS} -dylib -shared $^ -o $@

.PHONY: clean
clean:
	$(RM) $(OPTIMIZER) $(OBJs)

run:
	opt -enable-new-pm=0 -load ./lib/LoopWalk.so -loop-walk test/Loop.ll -disable-output

load:
	opt -enable-new-pm=0 -load ./lib/LoopWalk.so -loop-walk test/Loop.ll -o test/LoopOpt.bc
	llvm-dis test/LoopOpt.bc -o test/LoopOpt.ll
