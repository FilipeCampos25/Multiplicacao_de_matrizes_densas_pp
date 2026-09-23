# Programação Paralela (CCO085) — IESB 2026/2
# Uso: make            -> compila tudo
#      make openmp     -> só os exemplos OpenMP
#      make mpi        -> só os exemplos MPI
#      make clean      -> remove os binários

CC      = gcc
MPICC   = mpicc
CFLAGS  = -O2 -Wall
OMPFLAG = -fopenmp
LDLIBS  = -lm

BASE   = codigos/00_base
OMPDIR = codigos/01_openmp
MPIDIR = codigos/02_mpi

BASE_SRC = $(wildcard $(BASE)/*.c)
OMP_SRC  = $(wildcard $(OMPDIR)/*.c)
MPI_SRC  = $(wildcard $(MPIDIR)/*.c)

BASE_BIN = $(BASE_SRC:.c=)
OMP_BIN  = $(OMP_SRC:.c=)
MPI_BIN  = $(MPI_SRC:.c=)

all: base openmp mpi

base: $(BASE_BIN)
openmp: $(OMP_BIN)
mpi: $(MPI_BIN)

$(BASE)/%: $(BASE)/%.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

$(OMPDIR)/%: $(OMPDIR)/%.c
	$(CC) $(CFLAGS) $(OMPFLAG) -o $@ $< $(LDLIBS)

$(MPIDIR)/%: $(MPIDIR)/%.c
	$(MPICC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(BASE_BIN) $(OMP_BIN) $(MPI_BIN)

.PHONY: all base openmp mpi clean
