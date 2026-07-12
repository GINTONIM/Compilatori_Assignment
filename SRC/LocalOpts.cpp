#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

// Aggiungo questi include per lavorare con le costanti 
// e per fare controlli specifici sulle istruzioni matematiche.
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace {

// =========================================================================
// PASSO 1: Algebraic Identity (x + 0 = x, x * 1 = x)
// =========================================================================
struct AlgebraicIdentityPass : PassInfoMixin<AlgebraicIdentityPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    
    // flag per eventuali modifiche IR
    bool isModified = false;

    // Scorro tutti i Basic Block presenti nella Funzione F
    for (auto &BB : F) {
      
      // Uso un iteratore per scorrere tutte le istruzioni all'interno del Basic Block corrente.
      for (auto Iter = BB.begin(); Iter != BB.end(); ++Iter) {
        
        // Estraggo la reference all'istruzione corrente dall'iteratore.
        Instruction &I = *Iter;

        // -----------------------------------------------------------------
        // CONTROLLO 1: Addizione (x + 0 = x)
        // -----------------------------------------------------------------
        // verifico se l'istruzione che sto analizzando è un'addizione.
        if (I.getOpcode() == Instruction::Add) {
            
          // Li estraggo usando getOperand() e li salvo come puntatori generici a 'Value'.
          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);

          // Provo a fare il downcasting del secondo operando. 
          // se è una costante C punterà a quella costante, altrimenti dyn_cast restituirà nullptr.
          if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
            
            // verifico se il valore è zero.
            if (C->isZero()) {
              // L'addizione è inutile al suo posto passo direttamente il primo operando.
              I.replaceAllUsesWith(Op0);
              isModified = true;
            }
          } 
          else if (ConstantInt *C = dyn_cast<ConstantInt>(Op0)) {
            if (C->isZero()) {
              // In questo caso rimpiazzo tutti gli usi con il secondo operando.
              I.replaceAllUsesWith(Op1);
              isModified = true;
            }
          }
        }

        // -----------------------------------------------------------------
        // CONTROLLO 2: Moltiplicazione (x * 1 = x)
        // -----------------------------------------------------------------
        // Verifico se l'opcode corrisponde a una moltiplicazione.
        if (I.getOpcode() == Instruction::Mul) {
            
          // Estraggo i due operandi.
          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);

          // Faccio il downcasting per vedere se il secondo operando è una Costante Intera.
          if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
            // Uso il metodo isOne() di LLVM per vedere se la costante vale 1.
            if (C->isOne()) {
              // Pattern "x * 1" trovato. Rimpiazzo gli usi della moltiplicazione con il primo operando.
              I.replaceAllUsesWith(Op0);
              isModified = true;
            }
          } 
          else if (ConstantInt *C = dyn_cast<ConstantInt>(Op0)) {
            if (C->isOne()) {
              I.replaceAllUsesWith(Op1);
              isModified = true;
            }
          }
        }
      }//Ciclo Istruzioni BB
    }//Ciclo BB

    return isModified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

// =========================================================================
// PASSO 2: Strength Reduction (Generalizzato)
// =========================================================================
struct StrengthReductionPass : PassInfoMixin<StrengthReductionPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool isModified = false;

    for (auto &BB : F) {
      for (auto Iter = BB.begin(); Iter != BB.end(); ++Iter) {
        Instruction &I = *Iter;

        // -----------------------------------------------------------------
        // 1. OTTIMIZZAZIONE DELLA DIVISIONE
        // -----------------------------------------------------------------
        // Controllo se l'operazione è una divisione con o senza segno
        if (I.getOpcode() == Instruction::SDiv || I.getOpcode() == Instruction::UDiv) {
          Value *Op0 = I.getOperand(0); // Dividendo
          Value *Op1 = I.getOperand(1); // Divisore

          // Nella divisione l'ordine conta: la costante deve essere al secondo posto
          if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
            APInt Val = C->getValue(); // Estraggo il valore matematico (APInt)
            
            // Verifico che il divisore sia positivo e sia una potenza di 2 esatta
            if (Val.isStrictlyPositive() && Val.isPowerOf2()) {
              
              // Calcolo l'esponente per lo shift
              Constant *ShiftConst = ConstantInt::get(C->getType(), Val.logBase2());
              
              // Scelgo lo shift giusto: Aritmetico (mantiene il segno) per SDiv, Logico per UDiv
              Instruction::BinaryOps shiftType = (I.getOpcode() == Instruction::SDiv) ? Instruction::AShr : Instruction::LShr;
              
              // Creo e inserisco l'istruzione di shift
              Instruction *NewInst = BinaryOperator::Create(shiftType, Op0, ShiftConst);
              NewInst->insertAfter(&I);
              
              I.replaceAllUsesWith(NewInst);
              isModified = true;
            }
          }
        }

        // -----------------------------------------------------------------
        // 2. OTTIMIZZAZIONE DELLA MOLTIPLICAZIONE
        // -----------------------------------------------------------------
        if (I.getOpcode() == Instruction::Mul) {
          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);

          ConstantInt *C = nullptr;
          Value *X = nullptr; // Questa variabile conterrà l'operando non costante

          // Cerco la costante tenendo conto della proprietà commutativa
          if ((C = dyn_cast<ConstantInt>(Op1))) {
            X = Op0;
          } else if ((C = dyn_cast<ConstantInt>(Op0))) {
            X = Op1;
          }

          if (C) {
            APInt Val = C->getValue();

            if (Val.isStrictlyPositive() && !Val.isOne()) {
              
              // Esatta Potenza di 2 
              if (Val.isPowerOf2()) {
                Constant *ShiftConst = ConstantInt::get(C->getType(), Val.logBase2());
                Instruction *ShlInst = BinaryOperator::Create(Instruction::Shl, X, ShiftConst);
                
                ShlInst->insertAfter(&I);
                I.replaceAllUsesWith(ShlInst);
                isModified = true;
              }
              
              // Potenza di 2 MENO 1 
              else if ((Val + 1).isPowerOf2()) {
                APInt ValPlusOne = Val + 1;
                Constant *ShiftConst = ConstantInt::get(C->getType(), ValPlusOne.logBase2());
                
                // Creo lo shift
                Instruction *ShlInst = BinaryOperator::Create(Instruction::Shl, X, ShiftConst);
                ShlInst->insertAfter(&I);
                
                // Creo la seconda istruzione: la sottrazione ((x << 4) - x)
                Instruction *SubInst = BinaryOperator::Create(Instruction::Sub, ShlInst, X);
                SubInst->insertAfter(ShlInst); 
                
                // Rimpiazzo gli usi della vecchia mul con il risultato finale della sottrazione
                I.replaceAllUsesWith(SubInst); 
                isModified = true;
              }
              
              // Potenza di 2 PIÙ 1
              else if ((Val - 1).isPowerOf2()) {
                APInt ValMinusOne = Val - 1;
                Constant *ShiftConst = ConstantInt::get(C->getType(), ValMinusOne.logBase2()); // log2(8) = 3
                
                // shift
                Instruction *ShlInst = BinaryOperator::Create(Instruction::Shl, X, ShiftConst);
                ShlInst->insertAfter(&I);
                
                // Creo la seconda istruzione: l'addizione
                Instruction *AddInst = BinaryOperator::Create(Instruction::Add, ShlInst, X);
                AddInst->insertAfter(ShlInst);
                
                // Rimpiazzo gli usi della vecchia mul con il risultato finale dell'addizione
                I.replaceAllUsesWith(AddInst); 
                isModified = true;
              }
            }
          }
        }
      }
    }

    return isModified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

// =========================================================================
// PASSO 3: Multi-Instruction Optimization
// =========================================================================
struct MultiInstOptPass : PassInfoMixin<MultiInstOptPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool isModified = false;

    for (auto &BB : F) {
      for (auto Iter = BB.begin(); Iter != BB.end(); ++Iter) {
        Instruction &I = *Iter;

        // Cerchiamo una sottrazione: c = a - K
        if (I.getOpcode() == Instruction::Sub) {
          Value *OpA = I.getOperand(0);
          Value *OpK_sub = I.getOperand(1);

          // 1. Verifichiamo che il sottraendo sia una costante
          if (ConstantInt *K_sub = dyn_cast<ConstantInt>(OpK_sub)) {
            
            // 2. Verifichiamo che 'a' sia il risultato di un'istruzione precedente (Addizione)
            if (Instruction *AddI = dyn_cast<Instruction>(OpA)) {
              if (AddI->getOpcode() == Instruction::Add) {
                
                Value *AddOp0 = AddI->getOperand(0);
                Value *AddOp1 = AddI->getOperand(1);
                
                ConstantInt *K_add = nullptr;
                Value *B = nullptr;

                // 3. Gestione della COMMUTATIVITÀ dell'addizione (b + K oppure K + b)
                // Cerchiamo la costante nell'addizione e salviamo l'altro operando come 'b'
                if (ConstantInt *C = dyn_cast<ConstantInt>(AddOp0)) {
                  K_add = C;
                  B = AddOp1;
                } else if (ConstantInt *C = dyn_cast<ConstantInt>(AddOp1)) {
                  K_add = C;
                  B = AddOp0;
                }

                // 4. Verifichiamo che la costante dell'addizione e quella della sottrazione siano identiche
                if (K_add && K_add->getValue() == K_sub->getValue()) {
                  // Sostituiamo ogni uso del risultato della sottrazione con 'b'
                  I.replaceAllUsesWith(B);
                  isModified = true;
                }
              }
            }
          }
        }
        // Qui cerco una Addizione e verifico se uno degli operandi è una Sub
        else if (I.getOpcode() == Instruction::Add) {
          Value *OpA = I.getOperand(0); 
          Value *Op1 = I.getOperand(1);

          // Identifico quale operando è la costante e quale è la potenziale Sub
          ConstantInt *K_add = nullptr;
          Value *SubCandidate = nullptr;

          if (dyn_cast<ConstantInt>(OpA) && dyn_cast<Instruction>(Op1)) {
            K_add = dyn_cast<ConstantInt>(OpA);
            SubCandidate = Op1;
          } else if (dyn_cast<ConstantInt>(Op1) && dyn_cast<Instruction>(OpA)) {
            K_add = dyn_cast<ConstantInt>(Op1);
            SubCandidate = OpA;
          }

          if (K_add && SubCandidate) {
            if (Instruction *SubI = dyn_cast<Instruction>(SubCandidate)) {
              if (SubI->getOpcode() == Instruction::Sub) {
                // Estraggo 'b' (op0) e 'K' (op1) dalla sottrazione: a = b - K
                Value *B = SubI->getOperand(0);
                Value *OpK_sub = SubI->getOperand(1);

                if (ConstantInt *K_sub = dyn_cast<ConstantInt>(OpK_sub)) {
                  // Se le costanti coincidono, ho trovato (b - K) + K
                  if (K_add->getValue() == K_sub->getValue()) {
                    I.replaceAllUsesWith(B); // Il risultato finale è b
                    isModified = true;
                  }
                }
              }
            }
          }
        }
      }
    }
    return isModified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

// =========================================================================
// PASSO 4: Dead Code Elimination (DCE)
// =========================================================================
struct DeadCodeEliminationPass : PassInfoMixin<DeadCodeEliminationPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool isModified = false;
    bool changed = true;

    // L'algoritmo deve essere iterativo
    while (changed) {
      changed = false;

      for (auto &BB : F) {
        for (auto Iter = BB.begin(); Iter != BB.end(); ) {
          Instruction &I = *Iter++; // Incremento prima della cancellazione (post-incremento)

          // Un'istruzione è "Dead" se:
          // 1. Non è usata da nessuno (use_empty).
          // 2. Non ha effetti collaterali (es. non scrive in memoria, non fa I/O).
          // 3. Non è un terminatore (non è un salto o un return).
          if (I.use_empty() && !I.mayHaveSideEffects() && !I.isTerminator()) {
            I.eraseFromParent(); // Rimuove l'istruzione dalla BasicBlock
            changed = true;
            isModified = true;
          }
        }
      }
    }

    return isModified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

} // end namespace

// =========================================================================
// REGISTRAZIONE DEI PASSI NEL NEW PASS MANAGER
// =========================================================================
llvm::PassPluginLibraryInfo getLocalOptsPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Assignment1", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  // Mappo ogni stringa passata da riga di comando al rispettivo passo
                  if (Name == "algebraic-identity") {
                    FPM.addPass(AlgebraicIdentityPass());
                    return true;
                  }
                  if (Name == "strength-reduction") {
                    FPM.addPass(StrengthReductionPass());
                    return true;
                  }
                  if (Name == "multi-inst-opt") {
                    FPM.addPass(MultiInstOptPass());
                    return true;
                  }
                  if (Name == "dce-pass") {
                    FPM.addPass(DeadCodeEliminationPass());
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
  return getLocalOptsPluginInfo();
}