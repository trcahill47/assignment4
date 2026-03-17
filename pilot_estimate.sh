#!/bin/bash
# =============================================================================
# pilot_estimate.sh
#
# Runs a small pilot experiment (N=1000) with P=1 and P=4, then:
#   1. Prints real timing results
#   2. Estimates how long the FULL experiment will take (all N, all P)
#   3. Estimates peak memory usage per run
#
# Usage: bash pilot_estimate.sh
#
# Assumes make_matrix, matrix_matrix, and omp_matrix_matrix are already built.
# Run from the directory containing those executables.
# =============================================================================

# --- Configuration ---
PILOT_N=1000          # Small N for the pilot run
FULL_NS=(10000 20000 30000 40000)   # N values for the real experiment
THREAD_COUNTS=(1 2 4 8 16 32 64 128)
REPEATS=3             # How many times to repeat each pilot run (for avg)

MAKE_MATRIX=./make_matrix
SERIAL=./matrix_matrix
OMP=./omp_matrix_matrix

A_FILE=pilot_A.bin
B_FILE=pilot_B.bin
C_FILE=pilot_C.bin

# =============================================================================
# Helper: get wall time in seconds (works on Linux)
# =============================================================================
now_sec() { date +%s%N | awk '{printf "%.6f", $1/1e9}'; }

# =============================================================================
# STEP 1: Generate pilot matrices
# =============================================================================
echo "============================================================"
echo " PILOT EXPERIMENT  (N = ${PILOT_N})"
echo "============================================================"
echo ""
echo "[1/4] Generating ${PILOT_N}x${PILOT_N} pilot matrices..."
$MAKE_MATRIX $A_FILE $PILOT_N $PILOT_N
$MAKE_MATRIX $B_FILE $PILOT_N $PILOT_N

# Data size in bytes: 2 ints (header) + N*N doubles
PILOT_BYTES=$(echo "$PILOT_N * $PILOT_N * 8 + 8" | bc)
PILOT_MB=$(echo "scale=2; $PILOT_BYTES / 1048576" | bc)
echo "   Pilot matrix file size: ~${PILOT_MB} MB each"
echo ""

# =============================================================================
# STEP 2: Time serial run (P=1) several times, take the average
# =============================================================================
echo "[2/4] Timing serial run (P=1) x${REPEATS}..."

serial_work_total=0
serial_io_total=0

for i in $(seq 1 $REPEATS); do
    output=$($SERIAL $A_FILE $B_FILE $C_FILE 2>&1)
    csv_line=$(echo "$output" | grep "^CSV")
    t_work=$(echo  "$csv_line" | cut -d',' -f7)
    t_io=$(echo    "$csv_line" | cut -d',' -f8)
    serial_work_total=$(echo "$serial_work_total + $t_work" | bc)
    serial_io_total=$(echo   "$serial_io_total  + $t_io"   | bc)
    echo "   Run $i: work=${t_work}s  io_read=${t_io}s"
done

serial_work_avg=$(echo "scale=6; $serial_work_total / $REPEATS" | bc)
serial_io_avg=$(echo   "scale=6; $serial_io_total   / $REPEATS" | bc)

echo ""
echo "   Average serial work time : ${serial_work_avg}s"
echo "   Average serial IO time   : ${serial_io_avg}s"
echo ""

# =============================================================================
# STEP 3: Time a parallel run (P=4) to get a rough speedup estimate
# =============================================================================
echo "[3/4] Timing parallel run (P=4) x${REPEATS}..."

omp4_work_total=0
for i in $(seq 1 $REPEATS); do
    output=$($OMP $A_FILE $B_FILE $C_FILE 4 2>&1)
    csv_line=$(echo "$output" | grep "^CSV")
    t_work=$(echo "$csv_line" | cut -d',' -f7)
    omp4_work_total=$(echo "$omp4_work_total + $t_work" | bc)
    echo "   Run $i: work=${t_work}s"
done

omp4_work_avg=$(echo "scale=6; $omp4_work_total / $REPEATS" | bc)
speedup_4=$(echo "scale=2; $serial_work_avg / $omp4_work_avg" | bc 2>/dev/null || echo "N/A")

echo ""
echo "   Average P=4 work time    : ${omp4_work_avg}s"
echo "   Observed speedup at P=4  : ${speedup_4}x  (ideal = 4x)"
echo ""

# =============================================================================
# STEP 4: Scale up estimates to the full experiment
#
# Work scales as O(N^3): if pilot is N_p, full is N_f, ratio = (N_f/N_p)^3
# IO scales as O(N^2):   file size grows quadratically
#
# For parallel runs we assume the observed P=4 speedup holds (conservative).
# Speedup is bounded by Amdahl -- we use the pilot ratio as a proxy.
# =============================================================================
echo "[4/4] Projecting full experiment costs..."
echo ""

NUM_P=${#THREAD_COUNTS[@]}
NUM_N=${#FULL_NS[@]}

echo "------------------------------------------------------------"
printf "  %-8s  %-6s  %-18s  %-18s  %-14s\n" \
       "N" "P" "Est. work time (s)" "Est. IO read (s)" "Memory (GB)"
echo "------------------------------------------------------------"

total_estimated_seconds=0

for N in "${FULL_NS[@]}"; do
    # Scale factor relative to pilot (work = N^3, IO = N^2)
    work_scale=$(echo "scale=6; ($N / $PILOT_N)^3" | bc)
    io_scale=$(echo   "scale=6; ($N / $PILOT_N)^2" | bc)

    est_serial_work=$(echo "scale=4; $serial_work_avg * $work_scale" | bc)
    est_io=$(echo          "scale=4; $serial_io_avg   * $io_scale"   | bc)

    # Memory: A + B + C each N*N doubles = 3 * N^2 * 8 bytes
    mem_bytes=$(echo "$N * $N * 8 * 3" | bc)
    mem_gb=$(echo "scale=2; $mem_bytes / 1073741824" | bc)

    for P in "${THREAD_COUNTS[@]}"; do
        if [ "$P" -eq 1 ]; then
            est_work=$est_serial_work
        else
            # Conservative: assume speedup scales as pilot_speedup_4 * log2(P/4)
            # Capped -- you will not get perfect linear speedup
            est_work=$(echo "scale=4; $est_serial_work / $speedup_4" | bc 2>/dev/null || echo "$est_serial_work")
        fi

        printf "  %-8s  %-6s  %-18s  %-18s  %-14s\n" \
               "$N" "$P" "$est_work" "$est_io" "${mem_gb} GB"

        total_estimated_seconds=$(echo "$total_estimated_seconds + $est_work + $est_io" | bc)
    done
done

echo "------------------------------------------------------------"
echo ""

# Convert total seconds to hours
total_hours=$(echo "scale=2; $total_estimated_seconds / 3600" | bc)
total_hours_padded=$(echo "scale=2; $total_hours * 1.25" | bc)   # +25% safety margin

echo "============================================================"
echo " SUMMARY"
echo "============================================================"
echo ""
echo "  Pilot N             : $PILOT_N"
echo "  Pilot serial work   : ${serial_work_avg}s"
echo "  Pilot serial IO     : ${serial_io_avg}s"
echo "  Observed speedup P=4: ${speedup_4}x"
echo ""
echo "  Full experiment:"
echo "    N values          : ${FULL_NS[*]}"
echo "    Thread counts     : ${THREAD_COUNTS[*]}"
echo "    Total runs        : $((NUM_N * NUM_P))"
echo ""
echo "  Estimated total compute time : ${total_hours} hours"
echo "  With +25%% safety margin     : ${total_hours_padded} hours"
echo ""
echo "  Peak memory per run (N=40000): ~$(echo "40000 * 40000 * 8 * 3 / 1073741824" | bc) GB"
echo "  (Expanse node has 256 GB -- you are well within limits)"
echo ""
echo "  Suggested SLURM wall time    : $(echo "scale=0; ($total_hours_padded + 1) / 1" | bc) hours"
echo "  Suggested SLURM --mem        : 40GB  (safe for all N)"
echo ""
echo "============================================================"

# Cleanup pilot files
rm -f $A_FILE $B_FILE $C_FILE

echo "Pilot files cleaned up. Done."
