#!/bin/bash
# run_tests.sh - VMM test suite runner

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN_DIR="$PROJECT_ROOT/bin"
TRACE_DIR="$PROJECT_ROOT/traces"
OUTPUT_DIR="$SCRIPT_DIR/output"

VMM="$BIN_DIR/vmm"
TRACE_GEN="$BIN_DIR/trace_gen"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

# Ensure output directory exists
mkdir -p "$OUTPUT_DIR"

echo "========================================"
echo "  VMM Test Suite"
echo "========================================"
echo ""

# Test 1: Generate traces
info "Test 1: Generating sample traces"
if [ -f "$TRACE_GEN" ]; then
    "$TRACE_GEN" -t sequential -n 1000 -o "$TRACE_DIR/sequential.trace" > /dev/null 2>&1
    "$TRACE_GEN" -t random -n 5000 -o "$TRACE_DIR/random.trace" > /dev/null 2>&1
    "$TRACE_GEN" -t working_set -n 10000 -o "$TRACE_DIR/working_set.trace" > /dev/null 2>&1
    "$TRACE_GEN" -t locality -n 8000 -o "$TRACE_DIR/locality.trace" > /dev/null 2>&1
    "$TRACE_GEN" -t thrashing -n 15000 -o "$TRACE_DIR/thrashing.trace" > /dev/null 2>&1
    pass "Trace generation"
else
    fail "Trace generator not found"
fi

# Test 2: Run with FIFO algorithm
info "Test 2: FIFO replacement algorithm"
if "$VMM" -r 16 -p 4096 -t "$TRACE_DIR/sequential.trace" -a FIFO -T 16 \
    -o "$OUTPUT_DIR/fifo.json" --csv "$OUTPUT_DIR/fifo.csv" > "$OUTPUT_DIR/fifo.log" 2>&1; then
    # Check that page faults occurred
    if grep -q "Page Faults:" "$OUTPUT_DIR/fifo.log"; then
        pass "FIFO algorithm execution"
    else
        fail "FIFO algorithm - no page faults recorded"
    fi
else
    fail "FIFO algorithm execution failed"
fi

# Test 3: Run with LRU algorithm
info "Test 3: LRU replacement algorithm"
if "$VMM" -r 16 -p 4096 -t "$TRACE_DIR/sequential.trace" -a LRU -T 16 \
    -o "$OUTPUT_DIR/lru.json" --csv "$OUTPUT_DIR/lru.csv" > "$OUTPUT_DIR/lru.log" 2>&1; then
    if grep -q "Page Faults:" "$OUTPUT_DIR/lru.log"; then
        pass "LRU algorithm execution"
    else
        fail "LRU algorithm - no page faults recorded"
    fi
else
    fail "LRU algorithm execution failed"
fi

# Test 4: Run with Clock algorithm
info "Test 4: Clock (Second-Chance) algorithm"
if "$VMM" -r 16 -p 4096 -t "$TRACE_DIR/sequential.trace" -a CLOCK -T 16 \
    -o "$OUTPUT_DIR/clock.json" --csv "$OUTPUT_DIR/clock.csv" > "$OUTPUT_DIR/clock.log" 2>&1; then
    if grep -q "Page Faults:" "$OUTPUT_DIR/clock.log"; then
        pass "Clock algorithm execution"
    else
        fail "Clock algorithm - no page faults recorded"
    fi
else
    fail "Clock algorithm execution failed"
fi

# Test 5: Run with OPT algorithm
info "Test 5: OPT (Optimal) algorithm"
if "$VMM" -r 16 -p 4096 -t "$TRACE_DIR/sequential.trace" -a OPT -T 16 \
    -o "$OUTPUT_DIR/opt.json" --csv "$OUTPUT_DIR/opt.csv" > "$OUTPUT_DIR/opt.log" 2>&1; then
    if grep -q "Page Faults:" "$OUTPUT_DIR/opt.log"; then
        pass "OPT algorithm execution"
    else
        fail "OPT algorithm - no page faults recorded"
    fi
else
    fail "OPT algorithm execution failed"
fi

# Test 6: TLB effectiveness test
info "Test 6: TLB performance (small vs large TLB)"
"$VMM" -r 32 -p 4096 -t "$TRACE_DIR/locality.trace" -a LRU -T 8 \
    --csv "$OUTPUT_DIR/tlb_small.csv" > "$OUTPUT_DIR/tlb_small.log" 2>&1
"$VMM" -r 32 -p 4096 -t "$TRACE_DIR/locality.trace" -a LRU -T 128 \
    --csv "$OUTPUT_DIR/tlb_large.csv" > "$OUTPUT_DIR/tlb_large.log" 2>&1

# Extract TLB hit rates
TLB_SMALL_HITS=$(grep "Hit Rate:" "$OUTPUT_DIR/tlb_small.log" | grep "TLB" | awk '{print $3}' | tr -d '%')
TLB_LARGE_HITS=$(grep "Hit Rate:" "$OUTPUT_DIR/tlb_large.log" | grep "TLB" | awk '{print $3}' | tr -d '%')

if [ -n "$TLB_SMALL_HITS" ] && [ -n "$TLB_LARGE_HITS" ]; then
    # Larger TLB should have better hit rate
    if (( $(echo "$TLB_LARGE_HITS >= $TLB_SMALL_HITS" | bc -l) )); then
        pass "TLB size affects hit rate (Small: ${TLB_SMALL_HITS}%, Large: ${TLB_LARGE_HITS}%)"
    else
        fail "TLB hit rate unexpected (Small: ${TLB_SMALL_HITS}%, Large: ${TLB_LARGE_HITS}%)"
    fi
else
    fail "Could not extract TLB hit rates"
fi

# Test 7: Multi-process trace
info "Test 7: Multi-process workload"
if "$VMM" -r 32 -p 4096 -t "$TRACE_DIR/working_set.trace" -a CLOCK -T 32 \
    -V -o "$OUTPUT_DIR/multiproc.json" > "$OUTPUT_DIR/multiproc.log" 2>&1; then
    if grep -q "PER-PROCESS METRICS" "$OUTPUT_DIR/multiproc.log"; then
        pass "Multi-process execution with per-process metrics"
    else
        fail "Multi-process execution - no per-process metrics"
    fi
else
    fail "Multi-process execution failed"
fi

# Test 8: Thrashing scenario
info "Test 8: Thrashing scenario (high page fault rate)"
if "$VMM" -r 8 -p 4096 -t "$TRACE_DIR/thrashing.trace" -a FIFO -T 16 \
    --csv "$OUTPUT_DIR/thrashing.csv" > "$OUTPUT_DIR/thrashing.log" 2>&1; then
    # Extract page fault rate
    PF_RATE=$(grep "Fault Rate:" "$OUTPUT_DIR/thrashing.log" | awk '{print $3}' | tr -d '%')
    if [ -n "$PF_RATE" ]; then
        # Thrashing should have high page fault rate (> 10%)
        if (( $(echo "$PF_RATE > 10" | bc -l) )); then
            pass "Thrashing scenario (PF rate: ${PF_RATE}%)"
        else
            fail "Thrashing scenario - page fault rate too low (${PF_RATE}%)"
        fi
    else
        fail "Could not extract page fault rate"
    fi
else
    fail "Thrashing scenario execution failed"
fi

# Test 9: Two-level page table
info "Test 9: Two-level page table"
if "$VMM" -r 32 -p 4096 -t "$TRACE_DIR/random.trace" -a LRU -T 32 \
    --pt-type TWO_LEVEL -o "$OUTPUT_DIR/two_level.json" > "$OUTPUT_DIR/two_level.log" 2>&1; then
    if grep -q "Two-level" "$OUTPUT_DIR/two_level.log"; then
        pass "Two-level page table execution"
    else
        fail "Two-level page table not confirmed in output"
    fi
else
    fail "Two-level page table execution failed"
fi

# Test 10: Different page sizes
info "Test 10: Different page sizes (4KB vs 8KB)"
"$VMM" -r 32 -p 4096 -t "$TRACE_DIR/sequential.trace" -a CLOCK -T 32 \
    --csv "$OUTPUT_DIR/page_4k.csv" > "$OUTPUT_DIR/page_4k.log" 2>&1
"$VMM" -r 32 -p 8192 -t "$TRACE_DIR/sequential.trace" -a CLOCK -T 32 \
    --csv "$OUTPUT_DIR/page_8k.csv" > "$OUTPUT_DIR/page_8k.log" 2>&1

if [ -f "$OUTPUT_DIR/page_4k.csv" ] && [ -f "$OUTPUT_DIR/page_8k.csv" ]; then
    pass "Different page size configurations"
else
    fail "Page size configuration test failed"
fi

# Summary
echo ""
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi

