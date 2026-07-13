#include <stdio.h>

void test_dipendenza_negativa(int *a, int *b, int *c) {
    for (int i = 0; i < 100; i++) {
        a[i] = b[i] * 2;
    }

    for (int i = 0; i < 100; i++) {
        c[i] = a[i + 3] + 5; 
    }
}