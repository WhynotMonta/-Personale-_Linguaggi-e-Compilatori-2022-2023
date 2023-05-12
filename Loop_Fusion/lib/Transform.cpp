#include "LocalOpts.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <optional>

using namespace llvm;

bool blockContainsOnlyBranch(BasicBlock *block) {
  return dyn_cast<BranchInst>(block->begin());
}

bool loopsHaveSameTripCount(Loop *firstLoop, Loop* secondLoop, ScalarEvolution &SE) {
  return SE.getSmallConstantTripCount(firstLoop) == SE.getSmallConstantTripCount(secondLoop);
}

bool loopsHaveSameBounds(Loop *firstLoop, Loop* secondLoop, ScalarEvolution &SE) {
  auto firstLoopBound = firstLoop->getBounds(SE);
  
  if(firstLoopBound.hasValue()) {
    Loop::LoopBounds bound = firstLoopBound.getValue();
    bound.getInitialIVValue();
  }

  return true;
}

bool check(Loop *firstLoop, Loop* secondLoop, ScalarEvolution &SE) {
  return 
  blockContainsOnlyBranch(firstLoop->getExitBlock()) && 
  loopsHaveSameTripCount(firstLoop, secondLoop, SE) &&
  loopsHaveSameBounds(firstLoop, secondLoop, SE);
}

void replaceInductionVariables(Loop *prevLoop, Loop *currentLoop, ScalarEvolution &SE) {
  auto *firstInductionVariable = prevLoop->getInductionVariable(SE);
  auto *secondInductionVariable = currentLoop->getInductionVariable(SE);
  secondInductionVariable->replaceAllUsesWith(firstInductionVariable);
}

void loopFusion(Loop *prevLoop, Loop *currentLoop, ScalarEvolution &SE) {
  replaceInductionVariables(prevLoop, currentLoop, SE);

  // body del primo loop va nel body del secondo loop
  BasicBlock *firstBodyBlock = prevLoop->getLoopLatch()->getPrevNode();
  Instruction *firstLoopBodyBranch = firstBodyBlock->getTerminator();

  Instruction *secondLoopHeaderBranchInst = currentLoop->getHeader()->getTerminator();
  BasicBlock *secondLoopBodyBlock = secondLoopHeaderBranchInst->getSuccessor(0);

  // body del secondo loop va nel latch del primo loop
  BasicBlock *firstLoopLatchBlock = prevLoop->getLoopLatch();
  Instruction *secondLoopBodyBranchInst = secondLoopBodyBlock->getTerminator();

  // in caso di loop end, l'header del primo blocco va nell'exit del secondo loop
  BasicBlock *secondLoopExitBlock = currentLoop->getExitBlock();
  Instruction *firstLoopHeaderBranchInst = prevLoop->getHeader()->getTerminator();

  firstLoopBodyBranch->setSuccessor(0, secondLoopBodyBlock);
  firstLoopHeaderBranchInst->setSuccessor(1, secondLoopExitBlock);
  secondLoopBodyBranchInst->setSuccessor(0,firstLoopLatchBlock);

  //rimuovo latch del secondo loop
  BasicBlock *secondLoopLatchBlock = currentLoop->getLoopLatch();
  secondLoopLatchBlock->replaceAllUsesWith(currentLoop->getHeader());
  // secondLoopLatchBlock->eraseFromParent();


}

PreservedAnalyses TransformPass::run([[maybe_unused]] Function &F, FunctionAnalysisManager &AM) {

  auto &LoopInfo = AM.getResult<LoopAnalysis>(F);
  ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
  DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
  PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
  
  Loop *prevLoop = nullptr;
  for(auto &loop : LoopInfo.getLoopsInPreorder()) {
    if(prevLoop) {
      if(prevLoop->getExitBlock() == loop->getLoopPreheader()) {
        if(check(prevLoop, loop, SE)) {
          // polimerizzazione
          outs() << "loop fusion"<<"\n";
          loopFusion(prevLoop, loop, SE);
          break;
        }
      }
    } else {
      prevLoop = loop;
    }
  }
  return PreservedAnalyses::none();
}


