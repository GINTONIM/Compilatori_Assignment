#include <stdio.h>

void fusione_computazione(int *a, int *b, int *c, int *d) {
    // Primo Loop
    for (int i = 0; i < 100; i++) {
        a[i] = b[i] * 2;
    }

    // Secondo Loop (Adiacente, CFE, stesso trip count)
    for (int i = 0; i < 100; i++) {
        c[i] = a[i] + d[i];
    }
}