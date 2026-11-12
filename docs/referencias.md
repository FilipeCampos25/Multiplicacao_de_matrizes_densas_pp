# Referências técnicas

As referências abaixo fundamentam os conceitos efetivamente usados no projeto.
Não foi encontrada, no material versionado da disciplina, uma lista de livros
formalmente indicada pelo Prof. Rodrigo Gonçalves Pinto; por isso, nenhuma obra é
atribuída a ele. Os exemplos em `codigos/material_professor_CCO085/` são tratados
como material didático interno, separado da bibliografia publicada.

## Bibliografia

AMDAHL, Gene M. Validity of the single processor approach to achieving large
scale computing capabilities. In: *AFIPS Spring Joint Computer Conference*.
New York: ACM, 1967. p. 483–485. DOI:
[10.1145/1465482.1465560](https://doi.org/10.1145/1465482.1465560).

GOTO, Kazushige; VAN DE GEIJN, Robert A. Anatomy of high-performance matrix
multiplication. *ACM Transactions on Mathematical Software*, New York, v. 34,
n. 3, art. 12, p. 1–25, 2008. DOI:
[10.1145/1356052.1356053](https://doi.org/10.1145/1356052.1356053).

GUSTAFSON, John L. Reevaluating Amdahl's law. *Communications of the ACM*,
New York, v. 31, n. 5, p. 532–533, 1988. DOI:
[10.1145/42411.42415](https://doi.org/10.1145/42411.42415).

MESSAGE PASSING INTERFACE FORUM. *MPI: A Message-Passing Interface Standard*.
Version 4.1. [S. l.]: MPI Forum, 2023. Disponível em:
[MPI-4.1 oficial](https://www.mpi-forum.org/docs/mpi-4.1/mpi41-report.pdf).
Acesso em: 12 nov. 2026.

OPENMP ARCHITECTURE REVIEW BOARD. *OpenMP Application Programming Interface*.
Version 5.2. [S. l.]: OpenMP ARB, 2021. Disponível em:
[especificação oficial](https://www.openmp.org/wp-content/uploads/OpenMP-API-Specification-5-2.pdf).
Acesso em: 12 nov. 2026.

## Relação com o projeto

- Amdahl fundamenta os cenários hipotéticos e a discussão do limite de speedup.
- Gustafson contextualiza o aumento do problema na escalabilidade fraca; a série
  experimental deste projeto usa trabalho cúbico aproximadamente constante por
  trabalhador e não é apresentada como prova universal da lei.
- O padrão OpenMP sustenta o modelo de memória compartilhada, equipes, laços e
  escalonamento usados na implementação.
- O padrão MPI sustenta `Scatterv`, `Bcast`, `Gatherv`, ranks e coletivas.
- Goto e van de Geijn fundamentam a discussão sobre hierarquia de memória e
  localidade em multiplicação de matrizes; o projeto não reivindica implementar
  o algoritmo otimizado do artigo.
