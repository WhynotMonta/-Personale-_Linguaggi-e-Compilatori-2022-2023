#include "LocalOpts.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopAnalysisManager.h>

using namespace llvm;

PreservedAnalyses TransformPass::run([[maybe_unused]] Function &F, FunctionAnalysisManager &AM) {

  // Un semplice passo di esempio di manipolazione della IR
  // for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
  //   if (runOnFunction(*Iter)) {
  //     return PreservedAnalyses::none();
  //   }
  // }
  auto &LI = AM.getResult<LoopAnalysis>(F);
  outs() << "prova"<<"\n";
  return PreservedAnalyses::none();
}

