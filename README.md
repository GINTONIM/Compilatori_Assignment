# Assignment 1 - LLVM Optimizations

Questo repository contiene l'implementazione di 4 passi di ottimizzazione per LLVM (New Pass Manager).

## Struttura del Progetto
- `SRC/`: Contiene `LocalOpts.cpp` (codice dei passi) e `CMakeLists.txt`.
- `TEST/`: Contiene i file di test `.c` e la IR `.ll`.
- `BUILD/`: Cartella per la compilazione del plugin.

## 1. Compilazione
Assicurati di avere LLVM 19 installato. Dalla cartella principale:

```bash
mkdir BUILD
cd BUILD
cmake -DLT_LLVM_INSTALL_DIR=/usr/lib/llvm-19 ../SRC
make
```

## 2. Test delle Ottimizzazioni
Per testare i passi, segui questi step:

### A. Generazione della IR pulita
Per ogni file .c nella cartella TEST, genera la IR e applica mem2reg per trasformare gli accessi in memoria in accessi a registro

```Bash
cd TEST
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -S -c TestProg.c -o TestProg.O0.ll
opt-19 -passes=mem2reg TestProg.O0.ll -S -o TestProg.m2r.ll
```

### B. Esecuzione dei Passi
Puoi eseguire i passi singolarmente o in pipeline.

Esempio esecuzione singola (Algebraic Identity):

```Bash
opt-19 -S -load-pass-plugin ../BUILD/libLocalOpts.so -passes=algebraic-identity TestProg.m2r.ll -o TestProg.optimized.ll
```

Esempio esecuzione pipeline completa (tutti i passi + DCE):

```Bash
opt-19 -S -load-pass-plugin ../BUILD/libLocalOpts.so -passes="algebraic-identity,strength-reduction,multi-inst-opt,dce-pass" TestProg.m2r.ll -o TestProg.final.ll
```

## 3. Descrizione Passi
**algebraic-identity**: Semplifica x+0, 0+x, x1, 1x.

**strength-reduction**: Ottimizza mul/div per potenze di 2 e pattern K+1/K-1.

**multi-inst-opt**: Ottimizza (b+K)-K e (b-K)+K.

**dce-pass**: Elimina le istruzioni inutili (dead code) lasciate dagli altri passaggi.
