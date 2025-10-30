# VMM Simulator - Project Authors

## Development Team

### Aditya Pandey
**Role:** Lead Developer, Core Architecture  
**Contributions:**
- Core VMM orchestration and address translation pipeline
- Frame allocator implementation with bitmap optimization
- Swap manager and backing store simulation
- Main CLI interface and configuration system
- Integration and system-level testing

**Focus Areas:** Memory management, system design, performance optimization

---

### Kartik
**Role:** Memory Subsystems Developer  
**Contributions:**
- Page table implementation (single-level and two-level)
- Page replacement algorithms (FIFO, LRU, Clock, OPT)
- Page table entry flag management
- Algorithm performance analysis
- Documentation of replacement policies

**Focus Areas:** Page management, algorithmic optimization, data structures

---

### Vivek
**Role:** Performance & Metrics Engineer  
**Contributions:**
- TLB simulation with FIFO and LRU policies
- Comprehensive metrics collection system
- Multi-format reporting (JSON, CSV, console)
- Performance instrumentation and AMT calculation
- TLB optimization strategies

**Focus Areas:** Caching mechanisms, performance analysis, reporting systems

---

### Gaurang
**Role:** System Integration & Testing Lead  
**Contributions:**
- **Trace parsing and generation engine** - 5 sophisticated patterns
- **Main CLI interface** - professional argument parsing with validation
- **Logging and utilities system** - color-coded logging, bit manipulation helpers
- **Complete test suite** - 10 automated tests with quantitative validation
- **Comprehensive documentation** - README, DESIGN, API, TESTPLAN, Hinglish guides
- **System integration** - bringing all components together
- **Quality assurance** - ensuring production-ready code

**Focus Areas:** System integration, testing infrastructure, trace simulation, CLI/UX, documentation

**Key Achievement:** Made the entire system accessible and usable - from input (traces) to output (metrics) to testing (validation)

---

## Collaborative Work

All team members contributed to:
- Web-based GUI development and visualization
- Code reviews and pair programming
- Architecture discussions and design decisions
- Bug fixes and performance improvements
- User documentation and examples

---

## Project Timeline

**Phase 1:** Core Infrastructure (All)
- Basic VMM structure, build system, utilities

**Phase 2:** Memory Management (Aditya, Kartik)
- Frame allocator, page tables, replacement algorithms

**Phase 3:** Performance Features (Vivek)
- TLB implementation, metrics collection

**Phase 4:** Testing & Validation (Gaurang)
- Trace engine, test suite, documentation

**Phase 5:** Visualization (All)
- Web-based GUI, interactive controls

---

## Contact

For questions, suggestions, or contributions:
- Open an issue on the project repository
- Contact the development team

---

## Acknowledgments

Special thanks to:
- Operating Systems course instructors for guidance
- Open source community for tools and inspiration
- Research papers and OS textbooks for theoretical foundation

---

## License

This project is created for educational and research purposes.

**© 2025 Aditya Pandey, Kartik, Vivek, Gaurang**

Permission is granted to use, modify, and distribute this software for educational purposes, provided that proper credit is given to the original authors.

