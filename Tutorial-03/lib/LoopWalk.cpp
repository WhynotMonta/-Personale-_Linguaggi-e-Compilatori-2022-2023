#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/ValueTracking.h>

using namespace llvm;

namespace {

class LoopWalkPass final : public LoopPass {
public:
  static char ID;

  LoopWalkPass() : LoopPass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
  }

  virtual bool runOnLoop(Loop *L, LPPassManager &LPM) override {
    if(L->isLoopSimplifyForm()) 
    {
      outs() << "il loop e' in forma normalizzata" <<'\n';
    }  
    if(BasicBlock* BB=L->getLoopPreheader())
    {
      outs()<<"preheader: ";
      BB->print(outs());
    }
    for(Loop::block_iterator BI=L->block_begin(); BI!=L->block_end(); ++BI)
    {
      BasicBlock* BB=*BI;
      BB->print(outs());
    }

    return false; 
  }
};

char LoopWalkPass::ID = 0;
RegisterPass<LoopWalkPass> X("loop-walk","Loop Walk");

} // anonymous namespace

