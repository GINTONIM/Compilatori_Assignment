// Funzione per testare: x + 0 = x e x * 1 = x
int algebraic_identity(int x) {
    int a = x + 0;
    int b = a * 1;
    return b;
}

// Funzione per testare: 15 * x = (x << 4) - x e y = x / 8 = x >> 3
int strength_reduction(int x) {
    int a = x * 15;
    int b = a / 8;
    return b;
}

// Funzione per testare: a = b + 1, c = a - 1 => c = b
int multi_instruction(int b) {
    int a = b + 1;
    int c = a - 1;
    return c;
}