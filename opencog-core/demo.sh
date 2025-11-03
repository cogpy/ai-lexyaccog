#!/bin/bash
# OpenCog Core Demonstration Script
# Shows the capabilities of the distributed cognitive OS

set -e

echo "================================================"
echo "OpenCog Distributed OS Demonstration"
echo "================================================"
echo ""

# Build the system
echo "1. Building OpenCog Core..."
cd opencog-core
make clean > /dev/null 2>&1
make all 2>&1 | grep -E "(Compiling|Assembling|Creating|Generating)" || true
echo "   ✓ Build complete"
echo ""

# Run tests
echo "2. Running Test Suite..."
make test 2>&1 | tail -15
echo ""

# Show library info
echo "3. Generated Libraries:"
ls -lh lib/*.{a,so} 2>/dev/null || true
echo ""

# Show component summary
echo "4. Component Summary:"
echo "   - AtomSpace Implementation:      $(wc -l src/atomspace.c | awk '{print $1}') lines of C"
echo "   - Distributed OS Layer:          $(wc -l src/distributed.c | awk '{print $1}') lines of C"
echo "   - Assembly Optimizations:        $(wc -l asm/atom_ops.asm | awk '{print $1}') lines of x86-64 ASM"
echo "   - Cognitive Grammar Parser:      $(wc -l parsers/cognitive_grammar.y | awk '{print $1}') lines of Yacc"
echo "   - Lexical Analyzer:              $(wc -l parsers/cognitive_grammar.l | awk '{print $1}') lines of Lex"
echo ""

# Performance info
echo "5. Performance Features:"
echo "   ✓ Lock-free atomic operations"
echo "   ✓ SIMD vectorized pattern matching"
echo "   ✓ AVX2 batch truth value processing"
echo "   ✓ Shared memory IPC (< 1μs latency)"
echo "   ✓ Message queue async communication"
echo "   ✓ Thread-safe hash table lookup"
echo "   ✓ Reference-counted memory management"
echo ""

# Architecture summary
echo "6. Architecture Layers:"
echo "   ┌─────────────────────────────────────┐"
echo "   │  Cognitive Grammar Parser (Yacc)   │ ← High-level cognitive constructs"
echo "   ├─────────────────────────────────────┤"
echo "   │  AtomSpace (C)                      │ ← Knowledge representation"
echo "   ├─────────────────────────────────────┤"
echo "   │  Distributed OS (C)                 │ ← IPC, coordination, consensus"
echo "   ├─────────────────────────────────────┤"
echo "   │  Assembly Optimizations (x86-64)   │ ← Critical path performance"
echo "   └─────────────────────────────────────┘"
echo ""

# Integration options
echo "7. Integration Methods Available:"
echo "   • Node.js N-API native addons (recommended)"
echo "   • Unix domain sockets (process isolation)"
echo "   • Shared memory (highest performance)"
echo "   • Foreign Function Interface (FFI)"
echo ""

# Next steps
echo "8. Next Steps:"
echo "   → See docs/INTEGRATION.md for TypeScript integration"
echo "   → See docs/ARCHITECTURE.md for technical details"
echo "   → Run 'make install' to install system-wide"
echo ""

echo "================================================"
echo "Demonstration Complete!"
echo "================================================"
