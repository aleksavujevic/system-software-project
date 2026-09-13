# System Software Toolchain

Academic project developed as part of the System Software course.

A complete toolchain for a custom 32-bit processor architecture, consisting of an **assembler**, **linker**, and **emulator**.

```text
Assembly source (.s)
        ↓
    Assembler
        ↓
   Object file (.o)
        ↓
      Linker
        ↓
  Memory image (.hex)
        ↓
     Emulator
```

## Features

- Assembler with symbol tables, sections, relocations and literal pools
- Linker with symbol resolution, relocation processing and section placement
- Emulator supporting instruction execution, interrupts and memory-mapped terminal I/O

## Technologies

**C++17 · Flex · Bison · GNU Make · Linux**

## Build

Requirements: `g++`, `make`, `flex`, `bison`

```bash
make
```

This generates:

```text
assembler
linker
emulator
```

## Project Structure

```text
inc/      Header files
misc/     Flex and Bison specifications
src/      C++ source files
tests/    Assembly test programs
```

## Author

**Aleksa Vujević**
