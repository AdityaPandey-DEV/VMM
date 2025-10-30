#!/bin/bash
# VMM Simulator - Quick Start Script
# Authors: Aditya Pandey, Kartik, Vivek, Gaurang

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Project root
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# Print banner
print_banner() {
    echo -e "${PURPLE}"
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║                                                                ║"
    echo "║          🖥️  Virtual Memory Manager Simulator  🖥️              ║"
    echo "║                                                                ║"
    echo "║     Authors: Aditya Pandey, Kartik, Vivek, Gaurang           ║"
    echo "║                                                                ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Check and build if needed
check_and_build() {
    echo -e "${CYAN}🔧 Checking build status...${NC}"
    
    if [ ! -f "bin/vmm" ] || [ ! -f "bin/trace_gen" ]; then
        echo -e "${YELLOW}⚠️  Executables not found. Building project...${NC}"
        make clean
        make
        echo -e "${GREEN}✅ Build complete!${NC}"
    else
        echo -e "${GREEN}✅ Executables found!${NC}"
    fi
}

# Generate traces if missing
check_and_generate_traces() {
    echo -e "${CYAN}📊 Checking trace files...${NC}"
    
    local traces_exist=true
    for trace in sequential random working_set locality thrashing; do
        if [ ! -f "traces/${trace}.trace" ]; then
            traces_exist=false
            break
        fi
    done
    
    if [ "$traces_exist" = false ]; then
        echo -e "${YELLOW}⚠️  Trace files not found. Generating...${NC}"
        make traces
        echo -e "${GREEN}✅ Traces generated!${NC}"
    else
        echo -e "${GREEN}✅ Trace files found!${NC}"
    fi
}

# Start Web GUI
start_gui() {
    echo -e "${CYAN}🌐 Starting Web GUI...${NC}"
    echo ""
    echo -e "${GREEN}Web server starting on port 8000${NC}"
    echo -e "${YELLOW}Open your browser and go to:${NC}"
    echo -e "${BLUE}    http://localhost:8000${NC}"
    echo ""
    echo -e "${YELLOW}Press Ctrl+C to stop the server${NC}"
    echo ""
    
    cd gui
    python3 -m http.server 8000
}

# Start CLI with example
start_cli_example() {
    local ram=${1:-64}
    local algo=${2:-LRU}
    local trace=${3:-working_set}
    local tlb=${4:-64}
    
    echo -e "${CYAN}💻 Running CLI simulation...${NC}"
    echo ""
    echo -e "${YELLOW}Configuration:${NC}"
    echo -e "  RAM:       ${ram} MB"
    echo -e "  Algorithm: ${algo}"
    echo -e "  Trace:     ${trace}"
    echo -e "  TLB Size:  ${tlb} entries"
    echo ""
    
    ./bin/vmm -r "$ram" -p 4096 -t "traces/${trace}.trace" -a "$algo" -T "$tlb" -V
}

# Interactive CLI
start_cli_interactive() {
    echo -e "${CYAN}💻 Interactive CLI Mode${NC}"
    echo ""
    
    # Ask for RAM size
    echo -e "${YELLOW}Enter RAM size in MB (default: 64):${NC}"
    read -r ram
    ram=${ram:-64}
    
    # Ask for algorithm
    echo -e "${YELLOW}Choose algorithm:${NC}"
    echo "  1) FIFO"
    echo "  2) LRU (recommended)"
    echo "  3) CLOCK"
    echo "  4) APPROX_LRU"
    echo "  5) OPT"
    read -r algo_choice
    
    case $algo_choice in
        1) algo="FIFO" ;;
        2) algo="LRU" ;;
        3) algo="CLOCK" ;;
        4) algo="APPROX_LRU" ;;
        5) algo="OPT" ;;
        *) algo="LRU" ;;
    esac
    
    # Ask for trace
    echo -e "${YELLOW}Choose trace pattern:${NC}"
    echo "  1) sequential (best case)"
    echo "  2) random (worst case)"
    echo "  3) working_set (realistic)"
    echo "  4) locality (good locality)"
    echo "  5) thrashing (pathological)"
    read -r trace_choice
    
    case $trace_choice in
        1) trace="sequential" ;;
        2) trace="random" ;;
        3) trace="working_set" ;;
        4) trace="locality" ;;
        5) trace="thrashing" ;;
        *) trace="working_set" ;;
    esac
    
    # Ask for TLB size
    echo -e "${YELLOW}Enter TLB size (default: 64):${NC}"
    read -r tlb
    tlb=${tlb:-64}
    
    echo ""
    start_cli_example "$ram" "$algo" "$trace" "$tlb"
}

# Run benchmarks
run_benchmarks() {
    echo -e "${CYAN}🏃 Running benchmark suite...${NC}"
    echo ""
    
    mkdir -p benchmarks
    
    echo -e "${YELLOW}Running all algorithms with working_set trace...${NC}"
    for algo in FIFO LRU CLOCK APPROX_LRU OPT; do
        echo -e "${BLUE}  Testing $algo...${NC}"
        ./bin/vmm -r 64 -p 4096 -t traces/working_set.trace -a "$algo" -T 64 \
            --csv "benchmarks/${algo}_results.csv" --config-name "$algo" > /dev/null
    done
    
    echo ""
    echo -e "${GREEN}✅ Benchmarks complete!${NC}"
    echo -e "Results saved in: ${YELLOW}benchmarks/${NC}"
    echo ""
    echo -e "${CYAN}CSV files created:${NC}"
    ls -lh benchmarks/*.csv
}

# Run tests
run_tests() {
    echo -e "${CYAN}🧪 Running test suite...${NC}"
    echo ""
    make test
}

# Show menu
show_menu() {
    echo -e "${CYAN}Choose an option:${NC}"
    echo ""
    echo -e "  ${GREEN}1)${NC} 🌐 Start Web GUI (recommended for beginners)"
    echo -e "  ${GREEN}2)${NC} 💻 Run CLI with default settings"
    echo -e "  ${GREEN}3)${NC} ⚙️  Run CLI with custom settings (interactive)"
    echo -e "  ${GREEN}4)${NC} 🏃 Run performance benchmarks"
    echo -e "  ${GREEN}5)${NC} 🧪 Run test suite"
    echo -e "  ${GREEN}6)${NC} 📚 Show quick examples"
    echo -e "  ${GREEN}7)${NC} 🛠️  Build only (no run)"
    echo -e "  ${GREEN}8)${NC} 📖 Open documentation"
    echo -e "  ${GREEN}9)${NC} ❌ Exit"
    echo ""
    echo -e -n "${YELLOW}Enter choice [1-9]: ${NC}"
}

# Show examples
show_examples() {
    echo -e "${CYAN}📚 Quick Examples:${NC}"
    echo ""
    echo -e "${YELLOW}1. Compare algorithms:${NC}"
    echo -e "   ${BLUE}for algo in FIFO LRU CLOCK; do${NC}"
    echo -e "   ${BLUE}  ./bin/vmm -r 32 -t traces/random.trace -a \$algo --csv \${algo}.csv${NC}"
    echo -e "   ${BLUE}done${NC}"
    echo ""
    echo -e "${YELLOW}2. Test TLB impact:${NC}"
    echo -e "   ${BLUE}./bin/vmm -r 64 -t traces/locality.trace -T 16 -a LRU  # Small TLB${NC}"
    echo -e "   ${BLUE}./bin/vmm -r 64 -t traces/locality.trace -T 128 -a LRU # Large TLB${NC}"
    echo ""
    echo -e "${YELLOW}3. See thrashing:${NC}"
    echo -e "   ${BLUE}./bin/vmm -r 8 -t traces/thrashing.trace -a FIFO -V${NC}"
    echo ""
    echo -e "${YELLOW}4. Generate custom trace:${NC}"
    echo -e "   ${BLUE}./bin/trace_gen -t working_set -n 10000 -o custom.trace${NC}"
    echo -e "   ${BLUE}./bin/vmm -r 64 -t custom.trace -a LRU${NC}"
    echo ""
    echo -e "${YELLOW}5. Save results:${NC}"
    echo -e "   ${BLUE}./bin/vmm -r 128 -t traces/working_set.trace -a LRU \\${NC}"
    echo -e "   ${BLUE}    -o results.json --csv results.csv${NC}"
    echo ""
    echo -e "Press Enter to continue..."
    read -r
}

# Open documentation
open_docs() {
    echo -e "${CYAN}📖 Available Documentation:${NC}"
    echo ""
    echo "  📄 START_HERE.md         - Quick start guide"
    echo "  📄 README.md             - Complete documentation"
    echo "  📄 QUICKSTART.md         - 5-minute guide"
    echo "  📄 docs/DESIGN.md        - Architecture details"
    echo "  📄 docs/API.md           - Developer API"
    echo "  📄 docs/TESTPLAN.md      - Testing guide"
    echo "  📄 EXTENSIONS.md         - Future enhancements"
    echo ""
    echo "  📕 Individual Code Explanations (Hinglish):"
    echo "  📄 docs/ADITYA_CODE_EXPLAINED.md   - Core VMM, Frame, Swap"
    echo "  📄 docs/KARTIK_CODE_EXPLAINED.md   - Page Tables, Algorithms"
    echo "  📄 docs/VIVEK_CODE_EXPLAINED.md    - TLB, Metrics"
    echo "  📄 docs/GAURANG_CODE_EXPLAINED.md  - Trace, CLI, Testing"
    echo ""
    echo -e "${YELLOW}Opening START_HERE.md...${NC}"
    
    if command -v open &> /dev/null; then
        open START_HERE.md
    elif command -v xdg-open &> /dev/null; then
        xdg-open START_HERE.md
    else
        cat START_HERE.md | less
    fi
}

# Main function
main() {
    print_banner
    
    # Check if running with arguments
    if [ $# -gt 0 ]; then
        case "$1" in
            gui|web)
                check_and_build
                start_gui
                ;;
            cli)
                check_and_build
                check_and_generate_traces
                start_cli_example "${2:-64}" "${3:-LRU}" "${4:-working_set}" "${5:-64}"
                ;;
            benchmark|bench)
                check_and_build
                check_and_generate_traces
                run_benchmarks
                ;;
            test)
                check_and_build
                check_and_generate_traces
                run_tests
                ;;
            build)
                make clean
                make
                echo -e "${GREEN}✅ Build complete!${NC}"
                ;;
            help|--help|-h)
                echo "Usage: ./start.sh [command] [options]"
                echo ""
                echo "Commands:"
                echo "  gui|web          Start web GUI"
                echo "  cli [ram] [algo] [trace] [tlb]   Run CLI simulation"
                echo "  benchmark        Run performance benchmarks"
                echo "  test             Run test suite"
                echo "  build            Build project only"
                echo "  help             Show this help"
                echo ""
                echo "Examples:"
                echo "  ./start.sh gui"
                echo "  ./start.sh cli 128 LRU working_set 64"
                echo "  ./start.sh benchmark"
                ;;
            *)
                echo -e "${RED}Unknown command: $1${NC}"
                echo "Run './start.sh help' for usage"
                exit 1
                ;;
        esac
    else
        # Interactive mode
        check_and_build
        check_and_generate_traces
        
        while true; do
            echo ""
            show_menu
            read -r choice
            
            case $choice in
                1)
                    start_gui
                    ;;
                2)
                    start_cli_example 64 LRU working_set 64
                    ;;
                3)
                    start_cli_interactive
                    ;;
                4)
                    run_benchmarks
                    ;;
                5)
                    run_tests
                    ;;
                6)
                    show_examples
                    ;;
                7)
                    make clean
                    make
                    echo -e "${GREEN}✅ Build complete!${NC}"
                    ;;
                8)
                    open_docs
                    ;;
                9)
                    echo -e "${GREEN}Goodbye! 👋${NC}"
                    exit 0
                    ;;
                *)
                    echo -e "${RED}Invalid choice. Please enter 1-9${NC}"
                    ;;
            esac
        done
    fi
}

# Run main
main "$@"


