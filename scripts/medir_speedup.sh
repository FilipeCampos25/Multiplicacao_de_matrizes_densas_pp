#!/bin/bash
# Automação de benchmarks - Issue 07
# Uso: ./scripts/medir_speedup.sh [N] [REPETICOES] [THREADS_OMP] [PROCS_MPI]

set -euo pipefail

# Parâmetros com valores padrão
N=${1:-512}
REPETICOES=${2:-5}
THREADS_OMP=${3:-4}
PROCS_MPI=${4:-4}

ARQUIVO_CSV="resultados/benchmarks_automatizados.csv"

# Variáveis de controlo para os futuros códigos paralelos
export OMP_NUM_THREADS=$THREADS_OMP
MPI_CMD="mpirun -np $PROCS_MPI"

# Cria a pasta resultados se não existir
mkdir -p resultados

# Cria o cabeçalho do CSV se o arquivo não existir
if [ ! -f "$ARQUIVO_CSV" ]; then
    echo "versao,N,paralelismo,repeticao,tempo_s,checksum" > "$ARQUIVO_CSV"
fi

echo "A iniciar benchmarks para N=$N com $REPETICOES repetições..."

# 1. Execução do Baseline Sequencial
EXEC_SEQ="codigos/baselines_sequenciais_CCO085/01_matmul"

if [ -f "$EXEC_SEQ" ]; then
    echo "A executar Baseline Sequencial (com 1 execução de aquecimento)..."
    $EXEC_SEQ $N > /dev/null # Aquecimento não registado
    
    for i in $(seq 1 $REPETICOES); do
        SAIDA=$($EXEC_SEQ $N)
        
        # Filtra os valores impressos para capturar apenas os números
        CHECKSUM=$(echo "$SAIDA" | awk -F'checksum=' '{print $2}' | awk '{print $1}')
        TEMPO=$(echo "$SAIDA" | awk -F'tempo=' '{print $2}' | awk '{print $1}')
        
        if [ -n "$TEMPO" ] && [ -n "$CHECKSUM" ]; then
            echo "sequencial,$N,1,$i,$TEMPO,$CHECKSUM" >> "$ARQUIVO_CSV"
        else
            echo "Erro: Formato de saída inválido na repetição $i"
        fi
    done
else
    echo "Erro: Executável sequencial não encontrado em $EXEC_SEQ. Compila-o primeiro."
fi

# 2. Espaço reservado para OpenMP (será ativado quando o grupo entregar o executável)
# 3. Espaço reservado para MPI (será ativado quando o grupo entregar o executável)

echo "Testes concluídos! Resultados gravados em $ARQUIVO_CSV"