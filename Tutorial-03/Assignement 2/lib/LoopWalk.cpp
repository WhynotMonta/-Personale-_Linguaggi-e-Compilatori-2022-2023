#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Dominators.h>
#include <iostream>
#include <map>
#include <vector>

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
    std::map<Instruction*, bool> invariantMap;
    SmallVector<BasicBlock*> exitBlocks;
    std::vector<Instruction*> CMcandidates;
    std::vector<Instruction*> removable;
    
    DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    BasicBlock *preheader = L->getLoopPreheader();
    
    // cerchiamo le istruzioni loop-invariant
    for (auto &BI : L->getBlocks()) {
      for (Instruction &instr : *BI) {
        bool isInvariant = true;
        if(instr.isBinaryOp()) { 
          for(auto *Iter=instr.op_begin(); Iter!=instr.op_end();++Iter) {
            Value *operand=*Iter;
            check(operand, L, isInvariant, invariantMap);
          }
          invariantMap[&instr] = isInvariant;
        }
      }
    }

    L->getExitBlocks(exitBlocks);
    
    // scegliere le instruzioni per la code motion
    for(auto& invariantInst : invariantMap) {
      if(invariantInst.second) {
        BasicBlock* instBB = dyn_cast<Instruction>(invariantInst.first)->getParent();
        bool isExitDominating = true;
        bool isUsesDominating = true;

        // controlliamo che domini tutte le uscite
        for(auto& exitBlock : exitBlocks) {
          isExitDominating &= DT->dominates(instBB, exitBlock);
          if(!isExitDominating)
            break;
        }

        // controlliamo che domini tutti gli usi
        for(auto &use : invariantInst.first->uses()) {
          BasicBlock* useExitBlock = dyn_cast<Instruction>(use)->getParent();
          isUsesDominating &= DT->dominates(instBB, useExitBlock);
          if(!isUsesDominating)
            break;
        }

        if(isExitDominating && isUsesDominating)
          CMcandidates.push_back(invariantInst.first);
      }
    }

    // itero sul dominator tree in DFS
    for(auto node = GraphTraits<DominatorTree *>::nodes_begin(DT); node != GraphTraits<DominatorTree *>::nodes_end(DT); ++node) {
      BasicBlock *BB = node->getBlock();
      for(Instruction &instr : *BB) {
        // controllo se l'istruzione corrente è candidata alla code motion
        if(std::find(CMcandidates.begin(), CMcandidates.end(), &instr) != CMcandidates.end()) {
          // la aggiungo ad un array ordinato
          removable.push_back(&instr);
        }
      }
    }

    for(auto& inst : removable) {
      outs() <<*inst<<"\n";
      inst->removeFromParent();
      inst->insertBefore(preheader->getTerminator());
    }

    return false;
  }

  void check(Value* operand, Loop *L, bool &isInvariant, std::map<Instruction*, bool> invariantMap) {
    if (dyn_cast<ConstantInt>(operand))              
      return;
    Instruction* reachingDef = dyn_cast<Instruction>(operand);
    if(reachingDef) {
      BasicBlock* bb = reachingDef->getParent();
      if(L->contains(bb) && !invariantMap[reachingDef])
        isInvariant = false;
    }
  }
};


char LoopWalkPass::ID = 0;
RegisterPass<LoopWalkPass> X("loop-walk", "Loop Walk");

} // anonymous namespace
