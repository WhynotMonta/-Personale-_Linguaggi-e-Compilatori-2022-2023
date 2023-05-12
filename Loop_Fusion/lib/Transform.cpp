#include "LocalOpts.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/LoopAnalysisManager.h>

using namespace llvm;

bool blockContainsOnlyBranch(BasicBlock *block) {
  return dyn_cast<BranchInst>(block->begin());
}

bool loopsHaveSameTripCount(Loop *firstLoop, Loop* secondLoop, ScalarEvolution &SE) {
  return SE.getSmallConstantTripCount(firstLoop) == SE.getSmallConstantTripCount(secondLoop);
}

// non funziona, marongiu dice di skippare la prima parte di controlli e andare direttamente
// alla loop fusion
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
  PHINode *firstInductionVariable = dyn_cast<PHINode>(prevLoop->getHeader()->begin());
  PHINode *secondInductionVariable = dyn_cast<PHINode>(currentLoop->getHeader()->begin());
  secondInductionVariable->replaceAllUsesWith(firstInductionVariable);
  secondInductionVariable->eraseFromParent();
}

void loopFusion(Loop *prevLoop, Loop *currentLoop, ScalarEvolution &SE) {
  replaceInductionVariables(prevLoop, currentLoop, SE);

  // body del primo loop va nel body del secondo loop
  // vado a prendere l'ultima istruzione dell'ultimo blocco del 1* loop
  // e poi prendo il primo basick block del 2* loop, cioè il loopp interno quando la condizione del loop è true (getSuccessor(0))
  BasicBlock *firstBodyBlock = prevLoop->getLoopLatch()->getPrevNode();
  Instruction *firstLoopBodyBranch = firstBodyBlock->getTerminator();

  Instruction *secondLoopHeaderBranchInst = currentLoop->getHeader()->getTerminator();
  BasicBlock *secondLoopBodyBlock = secondLoopHeaderBranchInst->getSuccessor(0);

  // body del secondo loop va nel latch del primo loop
  // selezione il blocco latch del primo loop 
  // selezione l'ultima istruzione del primo blocco interno del 2* loop
  BasicBlock *firstLoopLatchBlock = prevLoop->getLoopLatch();
  Instruction *secondLoopBodyBranchInst = secondLoopBodyBlock->getTerminator();

  // in caso di loop end, l'header del primo blocco va nell'exit del secondo loop
  // prendo l'exit block del 2* loop e prendo la condizione dell'header del 1* loop
  BasicBlock *secondLoopExitBlock = currentLoop->getExitBlock();
  Instruction *firstLoopHeaderBranchInst = prevLoop->getHeader()->getTerminator();

  //rimuovo latch del secondo loop
  // prendo il blocco latch del 2* loop
  BasicBlock *secondLoopLatchBlock = currentLoop->getLoopLatch();

  // tutte le operazioni devono essere fatte alla fine
  firstLoopBodyBranch->setSuccessor(0, secondLoopBodyBlock);                      // metto la prima istr. del secondo loop dopo l'ultima istr. di branch del primo loop
  firstLoopHeaderBranchInst->setSuccessor(1, secondLoopExitBlock);                // setto che in caso di condizione falsa del 1* loop esco nell'exit block del 2* loop 
  secondLoopBodyBranchInst->setSuccessor(0,firstLoopLatchBlock);                  // setto che dopo l'ultima istruzione di branch del primo blocco nel 2* loop, in caso sia true si vada al latch del 1* loop

  // facoltativo, eliminare il latch del secondo loop
  secondLoopLatchBlock->replaceAllUsesWith(currentLoop->getHeader());
  secondLoopLatchBlock->eraseFromParent();
}

PreservedAnalyses TransformPass::run([[maybe_unused]] Function &F, FunctionAnalysisManager &AM) {

  auto &LoopInfo = AM.getResult<LoopAnalysis>(F);
  ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
  DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
  PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
  
  Loop *prevLoop = nullptr;
  for(auto &loop : LoopInfo.getLoopsInPreorder()) {
    if(prevLoop) {
      // controllo che i due loop siano adiacenti
      if(prevLoop->getExitBlock() == loop->getLoopPreheader()) {
        // ulteriori controllo (in realtà questa parte è ancora da definire)
        if(check(prevLoop, loop, SE)) {
          // polimerizzazione
          loopFusion(prevLoop, loop, SE);
          // break perchè in questo test ci sono solo 2 loop, verrà generalizzato a più loop
          break;
        }
      }
    } else {
      prevLoop = loop;
    }
  }
  return PreservedAnalyses::none();
}


