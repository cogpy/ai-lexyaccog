# OpenCog Distributed OS Core

## Overview

This is the low-level C/C++ implementation of OpenCog as a distributed operating system with cognitive grammar support. It provides the foundational layer for distributed cognitive processing using raw C, C++, Yacc, Lex, and assembly language optimizations.

## Architecture

```
opencog-core/
├── src/           # Core C/C++ source files
├── include/       # Header files
├── lib/           # Compiled libraries
├── parsers/       # Yacc/Lex grammar parsers
├── asm/           # Assembly optimizations
├── tests/         # Unit and integration tests
└── docs/          # Technical documentation
```

## Components

### 1. AtomSpace (C++)
Core distributed knowledge representation and storage system with:
- Lock-free data structures for concurrent access
- Memory-mapped shared memory for IPC
- SIMD optimizations for pattern matching

### 2. Cognitive Grammar Parser (Yacc/Lex)
Parser for cognitive grammar expressions supporting:
- Semantic role labeling
- Dependency parsing
- Cognitive construction grammar

### 3. Distributed OS Primitives (C)
Operating system level primitives:
- Process management and scheduling
- Inter-process communication (shared memory, message queues)
- Resource allocation and management
- Distributed coordination primitives

### 4. Assembly Optimizations (x86-64/ARM)
Critical path optimizations:
- SIMD vectorized operations
- Lock-free atomic operations
- Fast memory allocation/deallocation

## Building

```bash
make clean
make all
make test
```

## Integration

The C/C++ core integrates with the TypeScript layer via:
- Node.js Native Addons (N-API)
- Foreign Function Interface (FFI)
- Unix domain sockets for IPC
- Shared memory segments

## Performance Targets

- AtomSpace operations: < 100ns
- Grammar parsing: > 10,000 tokens/sec
- IPC latency: < 1μs
- Distributed coordination: < 10ms

## License

Eclipse Public License 2.0 OR GPL-2.0-only WITH Classpath-exception-2.0
