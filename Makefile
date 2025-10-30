# Virtual Memory Manager - Makefile
# Professional build system for VMM simulator

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g -MMD -MP
CFLAGS_DEBUG = -Wall -Wextra -std=c11 -Og -g3 -DDEBUG -fsanitize=address -fsanitize=undefined
CFLAGS_RELEASE = -Wall -Wextra -std=c11 -O3 -DNDEBUG -march=native

SRCDIR = src
OBJDIR = obj
BINDIR = bin
TESTDIR = tests
TRACEDIR = traces
DOCDIR = docs

TARGET = $(BINDIR)/vmm
TARGET_DEBUG = $(BINDIR)/vmm_debug
TRACE_GEN = $(BINDIR)/trace_gen

# Source files
SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/vmm.c \
          $(SRCDIR)/pagetable.c \
          $(SRCDIR)/frame.c \
          $(SRCDIR)/tlb.c \
          $(SRCDIR)/swap.c \
          $(SRCDIR)/replacement.c \
          $(SRCDIR)/trace.c \
          $(SRCDIR)/metrics.c \
          $(SRCDIR)/util.c

TRACE_GEN_SOURCES = $(SRCDIR)/trace_gen.c \
                    $(SRCDIR)/trace.c \
                    $(SRCDIR)/util.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TRACE_GEN_OBJECTS = $(TRACE_GEN_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPENDS = $(OBJECTS:.o=.d)

# Default target
.PHONY: all
all: dirs $(TARGET) $(TRACE_GEN)

# Debug build
.PHONY: debug
debug: CFLAGS = $(CFLAGS_DEBUG)
debug: dirs $(TARGET_DEBUG)

# Release build
.PHONY: release
release: CFLAGS = $(CFLAGS_RELEASE)
release: clean dirs $(TARGET)

# Create directories
.PHONY: dirs
dirs:
	@mkdir -p $(OBJDIR) $(BINDIR) $(TRACEDIR) $(TESTDIR)/output

# Link main VMM executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Link debug VMM executable
$(TARGET_DEBUG): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Link trace generator
$(TRACE_GEN): $(TRACE_GEN_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Include dependency files
-include $(DEPENDS)

# Run tests
.PHONY: test
test: $(TARGET) $(TRACE_GEN)
	@echo "Running test suite..."
	@bash $(TESTDIR)/run_tests.sh

# Run with valgrind for memory leak checking
.PHONY: valgrind
valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		$(TARGET_DEBUG) -r 64 -p 4096 -t $(TRACEDIR)/simple.trace -a FIFO -T 16

# Generate sample traces
.PHONY: traces
traces: $(TRACE_GEN)
	@echo "Generating sample traces..."
	$(TRACE_GEN) -t sequential -n 1000 -o $(TRACEDIR)/sequential.trace
	$(TRACE_GEN) -t random -n 5000 -o $(TRACEDIR)/random.trace
	$(TRACE_GEN) -t working_set -n 10000 -o $(TRACEDIR)/working_set.trace
	$(TRACE_GEN) -t locality -n 8000 -o $(TRACEDIR)/locality.trace
	$(TRACE_GEN) -t thrashing -n 15000 -o $(TRACEDIR)/thrashing.trace

# Format code
.PHONY: format
format:
	@echo "Formatting code with clang-format..."
	@find $(SRCDIR) -name "*.c" -o -name "*.h" | xargs clang-format -i -style=file

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Clean everything including traces and test output
.PHONY: distclean
distclean: clean
	rm -rf $(TRACEDIR)/*.trace $(TESTDIR)/output/*

# Show help
.PHONY: help
help:
	@echo "VMM Simulator - Build Targets:"
	@echo "  all       - Build release version (default)"
	@echo "  debug     - Build with debug symbols and sanitizers"
	@echo "  release   - Build optimized release version"
	@echo "  test      - Run test suite"
	@echo "  traces    - Generate sample trace files"
	@echo "  valgrind  - Run with valgrind memory checker"
	@echo "  format    - Format code with clang-format"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo ""
	@echo "Example usage:"
	@echo "  make && ./bin/vmm -r 128 -p 4096 -t traces/working_set.trace -a LRU -T 32"

