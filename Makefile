# Programacao Paralela (CCO085) - IESB 2026/2

.DEFAULT_GOAL := all

CXX      := g++
MPICXX   := mpic++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS   := -lm

BIN_DIR      := bin
BASE_SOURCE  := codigos/baselines_sequenciais_CCO085/01_matmul.cpp
BASE_BINARY  := $(BIN_DIR)/base/01_matmul
OPENMP_DIR   := codigos/01_openmp
MPI_DIR      := codigos/02_mpi
OPENMP_SRC   := $(wildcard $(OPENMP_DIR)/*.cpp)
MPI_SRC      := $(wildcard $(MPI_DIR)/*.cpp)
OPENMP_BIN   := $(patsubst $(OPENMP_DIR)/%.cpp,$(BIN_DIR)/openmp/%,$(OPENMP_SRC))
MPI_BIN      := $(patsubst $(MPI_DIR)/%.cpp,$(BIN_DIR)/mpi/%,$(MPI_SRC))
OPENMP_BENCH_BIN := $(addprefix $(BIN_DIR)/benchmark_openmp/matmul_,static dynamic guided)
BENCHMARK_ARGS ?=

AVAILABLE_TARGETS := base
ifneq ($(strip $(OPENMP_SRC)),)
AVAILABLE_TARGETS += openmp
endif
ifneq ($(strip $(MPI_SRC)),)
AVAILABLE_TARGETS += mpi
endif

all: $(AVAILABLE_TARGETS)
	@if test -z "$(OPENMP_SRC)"; then \
		printf '%s\n' "OpenMP: nenhuma fonte .cpp encontrada em $(OPENMP_DIR); ignorando."; \
	fi
	@if test -z "$(MPI_SRC)"; then \
		printf '%s\n' "MPI: nenhuma fonte .cpp encontrada em $(MPI_DIR); ignorando."; \
	fi

base: $(BASE_BINARY)

ifeq ($(strip $(OPENMP_SRC)),)
openmp:
	@printf '%s\n' "Erro: nenhuma fonte OpenMP .cpp encontrada em $(OPENMP_DIR)." >&2
	@exit 1
else
openmp: $(OPENMP_BIN)
endif

ifeq ($(strip $(MPI_SRC)),)
mpi:
	@printf '%s\n' "Erro: nenhuma fonte MPI .cpp encontrada em $(MPI_DIR)." >&2
	@exit 1
else
mpi: $(MPI_BIN)
endif

$(BASE_BINARY): $(BASE_SOURCE)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

$(BIN_DIR)/openmp/%: $(OPENMP_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -fopenmp -o $@ $< $(LDLIBS)

$(BIN_DIR)/mpi/%: $(MPI_DIR)/%.cpp
	@mkdir -p $(@D)
	$(MPICXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -rf $(BIN_DIR)

test: all
	bash ./scripts/test_matmul.sh

test-correctness: all
	MATMUL_BIN_DIR="$(BIN_DIR)" python3 tests/test_correctness.py

$(BIN_DIR)/benchmark_openmp/matmul_%: $(OPENMP_DIR)/matmul_openmp.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -fopenmp -DMATMUL_BENCH_SCHEDULE=$* -o $@ $< $(LDLIBS)
	@printf '%s\n' '$(CXX) $(CXXFLAGS) -fopenmp -DMATMUL_BENCH_SCHEDULE=$* -o $@ $< $(LDLIBS)' > $@.build.txt

benchmark-openmp-build: base openmp $(OPENMP_BENCH_BIN)

benchmark-openmp: benchmark-openmp-build
	MATMUL_BIN_DIR="$(BIN_DIR)" python3 scripts/benchmark_openmp.py $(BENCHMARK_ARGS)

.PHONY: all base openmp mpi test test-correctness benchmark-openmp-build benchmark-openmp clean
