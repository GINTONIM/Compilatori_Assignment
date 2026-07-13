#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct LoopFusionPass : public PassInfoMixin<LoopFusionPass> {

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        // Recuperiamo tutte le analisi necessarie dai rispettivi Analysis Manager
        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
        PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
        ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
        DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

        bool Changed = false;

        // Build up a worklist of loops to fuse. This is necessary as the
        // act of fusion creates new loops and can invalidate iterators
        // across the loops
        SmallVector<Loop *, 8> Worklist;
        for (Loop *L : LI) {
            Worklist.push_back(L);
        }

        // Se ci sono meno di 2 loop, non c'è nulla da fondere
        if (Worklist.size() < 2)
            return PreservedAnalyses::all();

        // Confrontiamo coppie di loop adiacenti nella worklist
        for (size_t i = 0; i < Worklist.size() - 1; ++i) {
            Loop *L0 = Worklist[i];
            Loop *L1 = Worklist[i + 1];

            // -----------------------------------------------------------------
            // CONDIZIONE 1: I due loop devono essere adiacenti
            // -----------------------------------------------------------------
            BasicBlock *L0Exit = L0->getExitBlock();
            BasicBlock *L1Preheader = L1->getLoopPreheader();
            BasicBlock *L1Exit = L1->getExitBlock();
            BasicBlock *L0Preheader = L0->getLoopPreheader();

            bool Adjacent = (L0Exit && L1Preheader && L0Exit == L1Preheader) ||
                            (L0Exit && L1Preheader && L0Exit->getUniqueSuccessor() == L1Preheader);

            if (!Adjacent) {
                bool InvertedAdjacent = (L1Exit && L0Preheader && L1Exit == L0Preheader) ||
                                        (L1Exit && L0Preheader && L1Exit->getUniqueSuccessor() == L0Preheader);
                
                if (InvertedAdjacent) {
                    // Sistemiamo l'ordine
                    std::swap(L0, L1);
                    Adjacent = true;
                }
            }

            if (!Adjacent) {
                errs() << "Loops non fusi: Condizione 1 (Adiacenza) fallita.\n";
                continue;
            }
            // -----------------------------------------------------------------
            // CONDIZIONE 2: Devono avere lo stesso numero di iterazioni (Trip Count)
            // -----------------------------------------------------------------
            const SCEV *L0TripCount = SE.getBackedgeTakenCount(L0);
            const SCEV *L1TripCount = SE.getBackedgeTakenCount(L1);
            if (isa<SCEVCouldNotCompute>(L0TripCount) || isa<SCEVCouldNotCompute>(L1TripCount) ||
                L0TripCount != L1TripCount) {
                errs() << "Loops non fusi: Condizione 2 (Trip Count identico) fallita.\n";
                continue;
            }

            // -----------------------------------------------------------------
            // CONDIZIONE 3: Devono essere Equivalenti nel Flusso di Controllo (CFE)
            // -----------------------------------------------------------------
            if (!DT.dominates(L0->getHeader(), L1->getHeader()) ||
                !PDT.dominates(L1->getHeader(), L0->getHeader())) {
                errs() << "Loops non fusi: Condizione 3 (Control Flow Equivalence) fallita.\n";
                continue;
            }

            // -----------------------------------------------------------------
            // CONDIZIONE 4: Assenza di dipendenze a distanza negativa (Backward)
            // Implementata confrontando gli Start delle SCEVAddRecExpr
            // -----------------------------------------------------------------
            bool hasNegativeDistanceDependence = false;
            for (BasicBlock *BB0 : L0->blocks()) {
                for (Instruction &I0 : *BB0) {
                    auto *ST = dyn_cast<StoreInst>(&I0);
                    if (!ST) continue; 
                    Value *StorePtr = ST->getPointerOperand();

                    for (BasicBlock *BB1 : L1->blocks()) {
                        for (Instruction &I1 : *BB1) {
                            auto *LD = dyn_cast<LoadInst>(&I1);
                            if (!LD) continue; 
                            Value *LoadPtr = LD->getPointerOperand();

                            // 1. Estraiamo le SCEV native senza forzare lo scope
                            const SCEV *StoreSCEV = SE.getSCEV(StorePtr);
                            const SCEV *LoadSCEV = SE.getSCEV(LoadPtr);

                            if (isa<SCEVCouldNotCompute>(StoreSCEV) || isa<SCEVCouldNotCompute>(LoadSCEV))
                                continue;

                            const auto *StoreAddRec = dyn_cast<SCEVAddRecExpr>(StoreSCEV);
                            const auto *LoadAddRec = dyn_cast<SCEVAddRecExpr>(LoadSCEV);

                            if (StoreAddRec && LoadAddRec) {
                                // 2. Estraiamo gli offset iniziali (costanti esterne ai loop)
                                const SCEV *StoreStart = StoreAddRec->getStart();
                                const SCEV *LoadStart = LoadAddRec->getStart();

                                // 3. Sottraiamo: StoreStart - LoadStart
                                const SCEV *Distance = SE.getMinusSCEV(StoreStart, LoadStart);

                                // 4. Se la distanza è una Costante, operano sicuramente sullo stesso array
                                if (auto *ConstDist = dyn_cast<SCEVConstant>(Distance)) {
                                    // Se la sottrazione è < 0 (es: a - (a+12) = -12)
                                    // la Load è più avanti della Store: dipendenza negativa
                                    if (ConstDist->getValue()->isNegative()) {
                                        hasNegativeDistanceDependence = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if (hasNegativeDistanceDependence) break;
                    }
                }
                if (hasNegativeDistanceDependence) break;
            }

            if (hasNegativeDistanceDependence) {
                errs() << "Loops non fusi: Condizione 4 (Dipendenza a distanza negativa) rilevata.\n";
                continue;
            }

            // -----------------------------------------------------------------
            // FASE DI TRASFORMAZIONE (La Fusione Reale)
            // -----------------------------------------------------------------
            errs() << "Fusione dei loop in corso...\n";

            BasicBlock *L0Header = L0->getHeader();
            BasicBlock *L1Header = L1->getHeader();
            BasicBlock *L0Body = L0->getBlocks()[1]; // Assumiamo un body singolo
            BasicBlock *L1Body = L1->getBlocks()[1];
            BasicBlock *L0Latch = L0->getLoopLatch();

            // 1. Sostituiamo gli usi della Induction Variable del Loop 2 con quella del Loop 1
            PHINode *IV0 = L0->getCanonicalInductionVariable();
            PHINode *IV1 = L1->getCanonicalInductionVariable();
            if (IV0 && IV1) {
                IV1->replaceAllUsesWith(IV0);
            }

            // 2. Modifichiamo il CFG per agganciare i blocchi
            // Il Body del Loop 1 ora deve saltare direttamente all'inizio del Body del Loop 2
            Instruction *L0BodyTerm = L0Body->getTerminator();
            L0BodyTerm->setSuccessor(0, L1Body);

            // Il Body del Loop 2 ora deve chiudere il ciclo saltando al Latch del Loop 1
            Instruction *L1BodyTerm = L1Body->getTerminator();
            L1BodyTerm->setSuccessor(0, L0Latch);

            // Rimuoviamo i vecchi collegamenti strutturali orfani del Loop 2
            L1Header->getTerminator()->eraseFromParent();
            new UnreachableInst(F.getContext(), L1Header);

            Changed = true;
            break; // Ci fermiamo per salvaguardare la consistenza della Worklist corrente
        }

        return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "LoopFusionPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "my-loop-fusion") {
                        FPM.addPass(LoopFusionPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}