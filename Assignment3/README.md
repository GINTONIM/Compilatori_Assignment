# Assignment 3: Loop-Invariant Code Motion (LICM)

Questo modulo implementa un passo di ottimizzazione personalizzato per il Middle-End di LLVM chiamato **LoopCodeMotion**. 
Il passo esegue un'analisi iterativa per identificare le istruzioni *loop-invariant* all'interno dei cicli e, se soddisfatte le condizioni di dominanza (o se la variabile è considerata *dead* all'uscita del ciclo), le sposta nel *preheader* del loop per ottimizzare le prestazioni del programma.

---

## 🛠️ Come fare la Build del Passo

Il progetto utilizza **CMake** per la configurazione e la generazione del file sorgente condiviso (`.so`).

### 1. Posizionati nella cartella dedicata alla build:
   ```bash
   cd Assignment3/BUILD
   ```
Genera i file di configurazione con CMake:
```bash
cmake -DLT_LLVM_INSTALL_DIR=/usr/lib/llvm-19 ../SRC
```

Compila il passo per generare la libreria condivisa:make
Nota: Al termine della compilazione, il file generato si troverà in BUILD/libLoopCodeMotion.so.

## 🧪 Come compilare ed eseguire i Test
Per testare il corretto funzionamento dell'ottimizzazione, spostati nella cartella dei test ed esegui la pipeline di compilazione in tre passaggi (utilizzando LLVM 19).
  
### Posizionati nella cartella dei test:
```Bash
cd ../TEST
```

### Passo 1: Generazione del codice intermedio (LLVM IR) non ottimizzato
Generiamo l'IR a partire dal file sorgente C (AdvancedLoop.c), disabilitando le ottimizzazioni di default di Clang per preservare la struttura originale del codice:  
```Bash
clang-19 -S -O0 -emit-llvm -Xclang -disable-O0-optnone AdvancedLoop.c -o AdvancedLoop.ll
```

### Passo 2: Promozione delle variabili in forma SSA (mem2reg)
Prima di applicare il nostro passo, è necessario ripulire il codice dalle istruzioni ridondanti di load e store in memoria, convertendo le variabili nei registri virtuali tramite il passo ufficiale mem2reg:
```Bash
opt-19 -passes=mem2reg -S AdvancedLoop.ll -o AdvancedLoop.m2r.ll
```
### Passo 3: Esecuzione del passo personalizzato di LICM
Eseguiamo finalmente il nostro passo caricando la libreria condivisa appena compilata tramite il flag -my-loop-motion:
```Bash
opt-19 -load-pass-plugin=../BUILD/libLoopCodeMotion.so -passes=my-loop-motion -S AdvancedLoop.m2r.ll -o AdvancedLoop.opt.ll
```