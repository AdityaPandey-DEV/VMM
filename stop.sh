#!/bin/bash
# VMM Simulator - Stop Script
# Authors: Aditya Pandey, Kartik, Vivek, Gaurang

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Print banner
print_banner() {
    echo -e "${PURPLE}"
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║                                                                ║"
    echo "║          🛑  VMM Simulator - Stop Script  🛑                   ║"
    echo "║                                                                ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Find and kill Python HTTP servers running on port 8000
stop_web_servers() {
    echo -e "${CYAN}🔍 Searching for web servers on port 8000...${NC}"
    
    # Find processes on port 8000
    local pids=$(lsof -ti:8000 2>/dev/null)
    
    if [ -z "$pids" ]; then
        echo -e "${YELLOW}ℹ️  No web server found on port 8000${NC}"
        return 0
    fi
    
    echo -e "${YELLOW}Found web server(s) running:${NC}"
    lsof -i:8000 2>/dev/null | grep LISTEN
    echo ""
    
    for pid in $pids; do
        local cmd=$(ps -p $pid -o comm= 2>/dev/null)
        echo -e "${BLUE}Stopping process $pid ($cmd)...${NC}"
        kill $pid 2>/dev/null
        
        # Wait a moment and check if process stopped
        sleep 1
        if kill -0 $pid 2>/dev/null; then
            echo -e "${YELLOW}Process still running, forcing termination...${NC}"
            kill -9 $pid 2>/dev/null
        fi
        
        if ! kill -0 $pid 2>/dev/null; then
            echo -e "${GREEN}✅ Stopped process $pid${NC}"
        else
            echo -e "${RED}❌ Failed to stop process $pid${NC}"
        fi
    done
}

# Find and kill any VMM simulation processes
stop_vmm_processes() {
    echo -e "${CYAN}🔍 Searching for running VMM simulations...${NC}"
    
    # Find VMM processes
    local pids=$(pgrep -f "bin/vmm" 2>/dev/null)
    
    if [ -z "$pids" ]; then
        echo -e "${YELLOW}ℹ️  No VMM simulation processes found${NC}"
        return 0
    fi
    
    echo -e "${YELLOW}Found VMM simulation(s) running:${NC}"
    ps -p $pids -o pid,comm,args 2>/dev/null
    echo ""
    
    for pid in $pids; do
        echo -e "${BLUE}Stopping VMM simulation (PID: $pid)...${NC}"
        kill $pid 2>/dev/null
        
        sleep 1
        if kill -0 $pid 2>/dev/null; then
            echo -e "${YELLOW}Process still running, forcing termination...${NC}"
            kill -9 $pid 2>/dev/null
        fi
        
        if ! kill -0 $pid 2>/dev/null; then
            echo -e "${GREEN}✅ Stopped VMM simulation${NC}"
        else
            echo -e "${RED}❌ Failed to stop VMM simulation${NC}"
        fi
    done
}

# Find and kill trace generation processes
stop_trace_generators() {
    echo -e "${CYAN}🔍 Searching for trace generators...${NC}"
    
    local pids=$(pgrep -f "bin/trace_gen" 2>/dev/null)
    
    if [ -z "$pids" ]; then
        echo -e "${YELLOW}ℹ️  No trace generator processes found${NC}"
        return 0
    fi
    
    echo -e "${YELLOW}Found trace generator(s) running:${NC}"
    ps -p $pids -o pid,comm,args 2>/dev/null
    echo ""
    
    for pid in $pids; do
        echo -e "${BLUE}Stopping trace generator (PID: $pid)...${NC}"
        kill $pid 2>/dev/null
        sleep 1
        
        if ! kill -0 $pid 2>/dev/null; then
            echo -e "${GREEN}✅ Stopped trace generator${NC}"
        else
            kill -9 $pid 2>/dev/null
            echo -e "${GREEN}✅ Force stopped trace generator${NC}"
        fi
    done
}

# Clean up temporary files
cleanup_temp_files() {
    echo -e "${CYAN}🧹 Cleaning up temporary files...${NC}"
    
    local cleaned=0
    
    # Remove output files if requested
    if [ "$1" == "--clean-output" ]; then
        if [ -d "output" ]; then
            echo -e "${BLUE}Removing output directory...${NC}"
            rm -rf output/*.json output/*.csv output/*.log 2>/dev/null
            cleaned=$((cleaned + $(find output -type f 2>/dev/null | wc -l)))
        fi
        
        if [ -d "benchmarks" ]; then
            echo -e "${BLUE}Removing benchmark results...${NC}"
            rm -rf benchmarks/*.csv benchmarks/*.json 2>/dev/null
            cleaned=$((cleaned + $(find benchmarks -type f 2>/dev/null | wc -l)))
        fi
    fi
    
    # Remove core dumps
    if ls core.* 1> /dev/null 2>&1; then
        echo -e "${BLUE}Removing core dumps...${NC}"
        rm -f core.*
        cleaned=$((cleaned + 1))
    fi
    
    # Remove swap files
    if ls *.swp 1> /dev/null 2>&1; then
        echo -e "${BLUE}Removing swap files...${NC}"
        rm -f *.swp
        cleaned=$((cleaned + 1))
    fi
    
    if [ $cleaned -gt 0 ]; then
        echo -e "${GREEN}✅ Cleaned $cleaned file(s)${NC}"
    else
        echo -e "${YELLOW}ℹ️  No temporary files to clean${NC}"
    fi
}

# Show running processes
show_running() {
    echo -e "${CYAN}📊 Current VMM-related processes:${NC}"
    echo ""
    
    local found=0
    
    # Web servers
    if lsof -ti:8000 &>/dev/null; then
        echo -e "${YELLOW}Web Servers (port 8000):${NC}"
        lsof -i:8000 2>/dev/null | grep LISTEN
        found=1
    fi
    
    # VMM simulations
    if pgrep -f "bin/vmm" &>/dev/null; then
        echo -e "${YELLOW}VMM Simulations:${NC}"
        ps -f -p $(pgrep -f "bin/vmm") 2>/dev/null
        found=1
    fi
    
    # Trace generators
    if pgrep -f "bin/trace_gen" &>/dev/null; then
        echo -e "${YELLOW}Trace Generators:${NC}"
        ps -f -p $(pgrep -f "bin/trace_gen") 2>/dev/null
        found=1
    fi
    
    if [ $found -eq 0 ]; then
        echo -e "${GREEN}No VMM-related processes running${NC}"
    fi
}

# Show help
show_help() {
    echo "Usage: ./stop.sh [options]"
    echo ""
    echo "Options:"
    echo "  --all              Stop all VMM processes (default)"
    echo "  --web              Stop web server only"
    echo "  --vmm              Stop VMM simulations only"
    echo "  --trace            Stop trace generators only"
    echo "  --clean-output     Also remove output files"
    echo "  --status           Show running processes"
    echo "  --help, -h         Show this help"
    echo ""
    echo "Examples:"
    echo "  ./stop.sh                    # Stop all"
    echo "  ./stop.sh --web              # Stop web server only"
    echo "  ./stop.sh --all --clean-output  # Stop all and clean output"
    echo "  ./stop.sh --status           # Check what's running"
}

# Main function
main() {
    print_banner
    
    local stop_web=false
    local stop_vmm=false
    local stop_trace=false
    local clean_output=false
    local show_status=false
    
    # Parse arguments
    if [ $# -eq 0 ]; then
        # Default: stop all
        stop_web=true
        stop_vmm=true
        stop_trace=true
    else
        for arg in "$@"; do
            case $arg in
                --all)
                    stop_web=true
                    stop_vmm=true
                    stop_trace=true
                    ;;
                --web)
                    stop_web=true
                    ;;
                --vmm)
                    stop_vmm=true
                    ;;
                --trace)
                    stop_trace=true
                    ;;
                --clean-output)
                    clean_output=true
                    ;;
                --status)
                    show_status=true
                    ;;
                --help|-h)
                    show_help
                    exit 0
                    ;;
                *)
                    echo -e "${RED}Unknown option: $arg${NC}"
                    echo "Run './stop.sh --help' for usage"
                    exit 1
                    ;;
            esac
        done
    fi
    
    # Show status if requested
    if [ "$show_status" = true ]; then
        show_running
        exit 0
    fi
    
    # Stop processes
    echo ""
    
    if [ "$stop_web" = true ]; then
        stop_web_servers
        echo ""
    fi
    
    if [ "$stop_vmm" = true ]; then
        stop_vmm_processes
        echo ""
    fi
    
    if [ "$stop_trace" = true ]; then
        stop_trace_generators
        echo ""
    fi
    
    # Cleanup if requested
    if [ "$clean_output" = true ]; then
        cleanup_temp_files --clean-output
        echo ""
    fi
    
    # Final summary
    echo -e "${GREEN}╔════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                                        ║${NC}"
    echo -e "${GREEN}║     ✅  All processes stopped!  ✅      ║${NC}"
    echo -e "${GREEN}║                                        ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════╝${NC}"
    echo ""
    
    # Show what's still running (if anything)
    if lsof -ti:8000 &>/dev/null || pgrep -f "bin/vmm" &>/dev/null || pgrep -f "bin/trace_gen" &>/dev/null; then
        echo -e "${YELLOW}⚠️  Some processes are still running:${NC}"
        show_running
    fi
}

# Run main
main "$@"


