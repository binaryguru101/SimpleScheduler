#include "header.h"
#include <stdio.h>

int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int n = 40; 
    for (int i = 0; i <= n; i += 10) { 
        printf("%d\n", fibonacci(i)); 
    }
    return 0;
}
