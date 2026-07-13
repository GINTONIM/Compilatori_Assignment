#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Support/raw_ostream.h"
#include <set>

using namespace llvm;

namespace {

struct LoopCodeMotion : public PassInfoMixin<LoopCodeMotion> {

    // Funzione helper per verificare se un'istruzione è Loop-Invariant
    bool isLoopInvariant(Instruction &Inst, Loop *L, const std::set<Instruction*> &InvariantSet) {
        // Se non produce un risultato (es. void) o legge/scrive memoria, la ignoriamo
        if (!Inst.getType()->isFirstClassType() || Inst.mayHaveSideEffects() || Inst.mayReadFromMemory())
            return false;

        // Controllo gli operandi dell'istruzione
        for (Use &U : Inst.operands()) {
            if (Instruction *OpInst = dyn_cast<Instruction>(U.get())) {
                // Se l'operando è calcolato DENTRO il loop, l'istruzione non è invariant
                // eccetto se fa già parte del nostro set di invariant calcolate ai passi precedenti
                if (L->contains(OpInst->getParent()) && InvariantSet.find(OpInst) == InvariantSet.end()) {
                    return false;
                }
            }
            // Se l'operando è una costante o un argomento della funzione (definiti fuori dal loop), va bene
        }
        return true; // Tutti gli operandi sono definiti fuori dal loop o sono già invarianti noti
    }

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);

        bool Changed = false;

        // Iteriamo su tutti i loop scoperti nella funzione
        for (Loop *L : LI) {
            
            // Recuperiamo il preheader. Se non c'è, non possiamo fare code motion
            BasicBlock *Preheader = L->getLoopPreheader();
            if (!Preheader) continue;

            // Recuperiamo i blocchi di uscita del loop per i controlli di dominanza
            SmallVector<BasicBlock *, 4> ExitBlocks;
            L->getExitingBlocks(ExitBlocks);

            // FASE 1: Ricerca ITERATIVA delle istruzioni loop-invariant a cascata
            std::set<Instruction*> InvariantSet;
            bool addedNewInvariant;
            do {
                addedNewInvariant = false;
                // Eseguiamo una ricerca sui blocchi del loop
                for (BasicBlock *BB : L->blocks()) {
                    for (Instruction &Inst : *BB) {
                        // Se l'istruzione non è ancora nel set e rispetta i criteri, la aggiungiamo
                        if (InvariantSet.find(&Inst) == InvariantSet.end() && isLoopInvariant(Inst, L, InvariantSet)) {
                            InvariantSet.insert(&Inst);
                            addedNewInvariant = true;
                        }
                    }
                }
            } while (addedNewInvariant); // Continua finché non smette di trovare nuove invariant

            // Vettore per mantenere l'ordine
            std::vector<Instruction*> Worklist;

            // FASE 2: Verifichiamo le condizioni per la Code Motion sulle invarianti trovate
            for (Instruction *Inst : InvariantSet) {
                BasicBlock *BB = Inst->getParent();
                
                // Condizione A: Il blocco dell'istruzione deve dominare tutte le uscite del loop
                bool dominatesAllExits = true;
                for (BasicBlock *ExitBB : ExitBlocks) {
                    if (!DT.dominates(BB, ExitBB)) {
                        dominatesAllExits = false;
                        break;
                    }
                }

                //oppure la variabile assegnata deve essere 'dead' all'esterno del loop
                bool isDeadOutsideLoop = true;
                for (User *U : Inst->users()) {
                    if (Instruction *UserInst = dyn_cast<Instruction>(U)) {
                        // Se c'è un uso fuori dal loop, la variabile non è morta fuori
                        if (!L->contains(UserInst->getParent())) {
                            isDeadOutsideLoop = false;
                            break;
                        }
                    }
                }

                // Se non domina le uscite E non è morta fuori, scartiamo l'istruzione
                if (!dominatesAllExits && !isDeadOutsideLoop) continue;

                // Condizione B: Assegna un valore a variabili non assegnate altrove
                // Lavorando sul codice LLVM IR dopo il passo 'mem2reg', 
                // il codice è in forma SSA (Static Single Assignment).
                // In SSA, ogni variabile è definita esattamente UNA sola volta. 
                // Pertanto, questa condizione è automaticamente soddisfatta dal framework LLVM.

                // Condizione C: Il blocco deve dominare tutti i blocchi nel loop che usano la variabile
                bool dominatesAllUses = true;
                for (User *U : Inst->users()) {
                    if (Instruction *UserInst = dyn_cast<Instruction>(U)) {
                        BasicBlock *UserBlock = UserInst->getParent();
                        // Controlliamo solo gli usi DENTRO il loop
                        if (L->contains(UserBlock)) {
                            if (!DT.dominates(BB, UserBlock)) {
                                dominatesAllUses = false;
                                break;
                            }
                        }
                    }
                }
                if (!dominatesAllUses) continue;

                // Se passa tutti i test, l'istruzione è candidata allo spostamento
                Worklist.push_back(Inst);
            }

            // FASE 3: Spostiamo le istruzioni candidate nel preheader rispettando le dipendenze
            bool movedAny;
            do {
                movedAny = false;
                for (auto it = Worklist.begin(); it != Worklist.end(); ) {
                    Instruction *InstToMove = *it;
                    
                    // Prima di spostarla, verifichiamo che tutte le istruzioni da cui dipende 
                    // siano state già spostate nel preheader
                    bool dependenciesMoved = true;
                    for (Use &U : InstToMove->operands()) {
                        if (Instruction *OpInst = dyn_cast<Instruction>(U.get())) {
                            if (L->contains(OpInst->getParent())) {
                                dependenciesMoved = false;
                                break;
                            }
                        }
                    }

                    if (dependenciesMoved) {
                        // Usiamo moveBefore posizionandolo prima del terminatore del preheader
                        InstToMove->moveBefore(Preheader->getTerminator());
                        errs() << "Spostata istruzione loop-invariant nel preheader: " << *InstToMove << "\n";
                        
                        it = Worklist.erase(it); // Rimuoviamo dalla Worklist l'istruzione appena spostata
                        movedAny = true;
                        Changed = true;
                    } else {
                        ++it; // Andiamo avanti e riproviamo al prossimo giro
                    }
                }
            } while (movedAny);
        }

        return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};

} // end namespace

// Configurazione standard per registrare il passo in LLVM (New Pass Manager)
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "LoopCodeMotion", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "my-loop-motion") {
                        FPM.addPass(LoopCodeMotion());
                        return true;
                    }
                    return false;
                });
        }
    };
}