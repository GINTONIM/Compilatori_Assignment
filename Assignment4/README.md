# Assignment 4: Loop Fusion Pass

Questo repository contiene l'implementazione di un'ottimizzazione per compilatori (LLVM Pass) volta a eseguire la **Loop Fusion** su codici in linguaggio C/LLVM IR.

## Obiettivo
L'obiettivo del progetto è implementare un passo di analisi e trasformazione che fonda due loop adiacenti in uno solo, a condizione che siano soddisfatti determinati criteri di correttezza (Adiacenza, Trip Count identico, Equivalenza del Control Flow e assenza di dipendenze negative).

## Requisiti
*   **LLVM 19**
*   **CMake** (versione 3.10 o superiore)
*   **Clang** (per la compilazione dei file di test in IR)

## Struttura del Progetto
*   `SRC/`: Contiene il codice sorgente del pass (`LoopFusion.cpp`).
*   `TEST/`: Contiene i file di test `.c` e i rispettivi file IR generati (`.ll`, `.m2r.ll`, `.opt.ll`).
*   `BUILD/`: Cartella di destinazione per la compilazione.
*   `CMakeLists.txt`: Script di configurazione per la build.

## Compilazione
Per compilare il pass, eseguire i seguenti comandi dalla cartella principale:

```bash
mkdir BUILD
cd BUILD
cmake ..
make
```

Il file compilato libLoopFusion.so verrà generato nella cartella BUILD.

## Esecuzione
Per eseguire il pass su un file sorgente, è necessario prima generare l'IR e prepararlo con le pass standard di LLVM, quindi invocare il proprio plugin:

### Generazione IR e semplificazione:

```Bash
clang-19 -S -O0 -emit-llvm -Xclang -disable-O0-optnone TEST/NomeTest.c -o TEST/NomeTest.ll
opt-19 -passes="mem2reg" -S TEST/NomeTest.ll -o TEST/NomeTest.m2r.ll
```

### Esecuzione della Loop Fusion:

```Bash
opt-19 -load-pass-plugin=../BUILD/libLoopFusion.so -passes=my-loop-fusion -S TEST/NomeTest.m2r.ll -o TEST/NomeTest.opt.ll
```
## Test inclusi
`FusionTest.c`: Test case positivo in cui due loop adiacenti vengono correttamente fusi.

`NegativeDependence.c`: Test case negativo in cui la fusione viene bloccata a causa di una dipendenza a distanza negativa (a[i+3]).
