#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
  
namespace {

struct LocalOpts: PassInfoMixin<LocalOpts> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    
    bool isModified = false;

    // Iteriamo su tutti i Basic Block della funzione
    for (auto &BB : F) {
        
        // Iteriamo sulle singole istruzioni del blocco
        for (auto Iter = BB.begin(); Iter != BB.end(); ++Iter) {
            Instruction &I = *Iter;

            // Cerchiamo le moltiplicazioni
            if(I.getOpcode() == Instruction::Mul) {
                Value *Op0 = I.getOperand(0);
                Value *Op1 = I.getOperand(1);

                ConstantInt *ConstOp = nullptr;
                Value *OtherOp = nullptr;

                // Controlliamo se c'è una costante nel secondo operando
                if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
                    ConstOp = C;
                    OtherOp = Op0;
                } 
                // Altrimenti, controlliamo il primo operando
                else if (ConstantInt *C = dyn_cast<ConstantInt>(Op0)) {
                    ConstOp = C;
                    OtherOp = Op1;
                }

                // Se abbiamo trovato una costante ed è una potenza di 2
                if (ConstOp && ConstOp->getValue().isPowerOf2()) {
                    
                    // Calcoliamo lo shift
                    uint64_t shiftAmount = ConstOp->getValue().logBase2();
                    Constant *ShiftConst = ConstantInt::get(ConstOp->getType(), shiftAmount);
                    
                    // Creiamo la nuova istruzione Shift Left (shl)
                    Instruction *NewInst = BinaryOperator::Create(
                        Instruction::Shl, OtherOp, ShiftConst);
                    
                    // La inseriamo e rimpiazziamo i vecchi usi
                    NewInst->insertAfter(&I);
                    I.replaceAllUsesWith(NewInst);
                    
                    isModified = true;
                }
            }
        }
    }

    // Segnaliamo a LLVM se abbiamo modificato il codice o no
    return isModified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};
}

llvm::PassPluginLibraryInfo getTestPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "LocalOpts", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "local-opts") {
                    FPM.addPass(LocalOpts());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize TestPass when added to the pass pipeline on the
// command line, i.e. via '-passes=test-pass'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getTestPassPluginInfo();
}