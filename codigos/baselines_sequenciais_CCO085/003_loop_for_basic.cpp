/*---------------------------------------
Paralelizar de for básico com OpenMP
-----------------------------------------*/

/*-----------------------------------------
 resolver a expressão: a = x*x + y*y + z*z
 para vetores x, y, z e armazenar em a.
-------------------------------------------*/

/*--------------------------------------------
 Primeiro, a versão sequencial (sem paralelismo)
----------------------------------------------*/
#include <iostream>
#include <vector>

int main() {
    const int N = 10;   // tamanho dos vetores
    std::vector<int> x(N), y(N), z(N), a(N);

    // Inicializa valores simples
    for (int i = 0; i < N; i++) {
        x[i] = i;
        y[i] = i + 1;
        z[i] = i + 2;
    }
     std::cout << "Calculando a = x*x + y*y + z*z para vetores de tamanho " << N << "\n";

    // Calcula a[i] = x[i]^2 + y[i]^2 + z[i]^2
    for (int i = 0; i < N; i++) {
        a[i] = x[i]*x[i] + y[i]*y[i] + z[i]*z[i]; // expressão a ser paralelizada
        std::cout << "i = " << i
                  << " | x[i] = " << x[i]
                  << " | y[i] = " << y[i]
                  << " | z[i] = " << z[i]
                  << " | a[i] = " << a[i]
                  << std::endl;
    }
    
        
    std::cout << "\nResumo dos resultados:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "  i |   x[i] |   y[i] |   z[i] |   a[i]\n";
    std::cout << "----------------------------------------\n";
    for (int i = 0; i < N; i++) {
        std::cout << " " << i
                  << "  | " << x[i]
                  << "     | " << y[i]
                  << "     | " << z[i]
                  << "     | " << a[i]
                  << std::endl;
    }
   std::cout << "----------------------------------------\n";
    
}
