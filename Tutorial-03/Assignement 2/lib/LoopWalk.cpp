#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Dominators.h>
#include <map>

using namespace llvm;

namespace {

class LoopWalkPass final : public LoopPass {
  public:
    static char ID;

  LoopWalkPass() : LoopPass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll(); //indico di preservare i risutati di questo passo di analisi
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>(); 
  }

  virtual bool runOnLoop(Loop *L, LPPassManager &LPM) override {
    DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    BasicBlock *preheader = L->getLoopPreheader();
    std::map<Instruction*, bool> invariantMap;
    
    for (Loop::block_iterator BI = L->block_begin(); BI != L->block_end(); ++BI) {
      BasicBlock *actualBB = *BI;
      for (Instruction &instr : *actualBB) {
        bool isInvariant = true;
        if(instr.isBinaryOp() ) { 
          outs()<<"Istruzione:"<<instr<<'\n';
          for(auto *Iter=instr.op_begin(); Iter!=instr.op_end();++Iter) {
            Value *operand=*Iter;
            outs()<<"Istruzione che definisce l'operando:"<<*operand<<'\n';
            check(operand, L, isInvariant);
          }
          outs() << isInvariant <<"\n";
        }
      }
    }
    return false;
  }

  void check(Value* operand, Loop *L, bool &isInvariant) {
    if (dyn_cast<ConstantInt>(operand)){              
      return;
    Instruction* reachingDef = dyn_cast<Instruction>(operand);
    if(reachingDef) {
      BasicBlock* bb = reachingDef->getParent();
      if(!L->contains(bb))
        isInvariant = false;
    }
  }
};



char LoopWalkPass::ID = 0;
RegisterPass<LoopWalkPass> X("loop-walk", "Loop Walk");

} // anonymous namespace
