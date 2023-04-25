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
    if(L->isLoopSimplifyForm())  {
      outs() << "il loop e' in forma normalizzata" <<'\n';
    }  

    if(BasicBlock* BB=L->getLoopPreheader()) {
      outs()<<"\n il preheader è :";
      BB->print(outs());
      outs()<<"\n";
    }

    for(Loop::block_iterator BI=L->block_begin(); BI!=L->block_end(); ++BI) {
      BasicBlock* BB=*BI;
      // BB->print(outs());
      for(auto i=BB->begin(); i!=BB->end();++i) {
        Instruction &Inst=*i;

        if(Inst.getOpcode()==Instruction::Sub) {
          for(auto *Iter=Inst.op_begin(); Iter!=Inst.op_end();++Iter) {
            Value *Operand=*Iter;
            if(!dyn_cast<ConstantInt>(Operand)) {  
              outs()<<"Istruzione che definisce l'operando:"<<*Operand<<'\n';
              outs()<<"\nIl suo basick block è:";
              dyn_cast<Instruction>(Iter)->getParent()->print(outs());
            }
          }
          outs()<<'\n';
        }  
      }
     
    }
    return false; 
  }
};

char LoopWalkPass::ID = 0;
RegisterPass<LoopWalkPass> X("loop-walk","Loop Walk");

} // anonymous namespace

