#include <stdio.h>

void test_licm_avanzato(int a, int b, int c) {
    int i = 0;
    int x, y, z;
    int sum = 0;

    while (i < 100) {
        // 'x' è invariant perché 'a' e 'b' sono definiti fuori dal loop.
        x = a + b;
        
        // 'y' è invariant, MA dipende da 'x'. 
        y = x + c;

        // L'istruzione 'z = ...' si trova dentro un branch condizionale.
        // Pertanto, il suo Basic Block NON domina tutte le uscite del loop.
        if (i % 2 == 0) {
            // 'z' è matematicamente invariante (dipende da y e a).
            // Dato che 'z' non viene mai letto o stampato fuori dal loop,
            // è considerato "dead" (morto) all'esterno.
            z = y * a;
            sum += z;
        } else {
            sum += y;
        }

        i++;
    }

    // Stampa di 'sum' per evitare che il compilatore elimini tutto per "dead code elimination".
    printf("Risultato finale: %d\n", sum);
}

int main() {
    test_licm_avanzato(5, 10, 15);
    return 0;
}