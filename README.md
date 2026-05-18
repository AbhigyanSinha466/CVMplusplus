# CVM++ — Stack-Based VM & Custom Scripting Language

## Overview

CVM++ is a complete, from-scratch implementation of:

1. A **custom scripting language** (`.cvm` files)
2. A **recursive-descent compiler** that translates source code to proprietary bytecode
3. A **stack-based virtual machine** that executes that bytecode

The entire pipeline — from raw text to execution result — is implemented in modern C++17 with zero dependencies beyond the standard library.

```
Source Code (.cvm)
      │
      ▼
  [Lexer]     → Tokens       (lexer.hpp / lexer.cpp)
      │
      ▼
  [Parser]    → AST          (parser.hpp / parser.cpp, ast.hpp / ast.cpp)
      │
      ▼
  [Compiler]  → Bytecode     (compiler.hpp / compiler.cpp, bytecode.hpp)
      │
      ▼
  [VM]        → Execution    (vm.hpp / vm.cpp)
```

---

## Project Structure

```
cvm++/
├── CMakeLists.txt          # Build system
├── README.md
├── include/                # Header files
│   ├── ast.hpp             # AST node definitions
│   ├── bytecode.hpp        # ISA definitions
│   ├── compiler.hpp        # Compiler interface
│   ├── lexer.hpp           # Lexer interface
│   ├── parser.hpp          # Parser interface
│   └── vm.hpp              # VM interface
├── src/                    # Implementation files
│   ├── main.cpp            # CLI entry point (REPL + file runner)
│   ├── ast.cpp             # AST printer implementation
│   ├── bytecode.cpp        # Bytecode helpers
│   ├── compiler.cpp        # AST → Bytecode compiler
│   ├── lexer.cpp           # Tokenizer implementation
│   ├── parser.cpp          # Recursive descent parser
│   └── vm.cpp              # Stack-based execution engine
└── tests/                  # Sample script files
    ├── hello.cvm           # Hello world / literals
    ├── arithmetic.cvm      # Arithmetic operators + precedence
    ├── factorial.cvm       # while loop: iterative factorial
    ├── fibonacci.cvm       # while loop: Fibonacci sequence
    ├── fizzbuzz.cvm        # Nested if/else: FizzBuzz 1–20
    ├── booleans.cvm        # Boolean logic + comparison operators
    └── input_demo.cvm      # Interactive I/O with `input` keyword
```

---

## Building

### Prerequisites

| Tool | Minimum Version | Notes |
|------|----------------|-------|
| CMake | 3.16+ | `brew install cmake` on macOS |
| C++ Compiler | C++17 support | Clang 10+, GCC 9+, Apple Clang 12+ |

### Build Steps

```bash
# 1. Create and enter the build directory
mkdir build && cd build

# 2. Configure (Release mode — optimised binary)
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. Compile
cmake --build . --parallel

# The binary is at: build/cvm  (or build/cvm.exe on Windows)
```

### Quick Build (no CMake)

If you just want to compile fast without CMake:

```bash
g++ -std=c++17 -O2 -Wall src/*.cpp -o cvm
# or with Clang:
clang++ -std=c++17 -O2 -Wall src/*.cpp -o cvm
```

---

## Usage

### Run a Script File

```bash
./build/cvm tests/factorial.cvm
```

### Debug Mode (`-d`)

Prints the full token list, AST tree, and bytecode disassembly before executing:

```bash
./build/cvm -d tests/factorial.cvm
```

### Interactive REPL

```bash
./build/cvm
# CVM++ Interactive REPL
# cvm> let x = 10;
# cvm> let y = x * 2 + 5;
# cvm> print y;
# 25
# cvm> exit
```

---

## The CVM++ Language

### Data Types

| Type    | Literals          | Notes                         |
|---------|-------------------|-------------------------------|
| Integer | `0`, `42`, `1000` | 64-bit signed (`int64_t`)     |
| Boolean | `true`, `false`   | Stored as C++ `bool`           |

### Variables

```cvm
let x = 42;         // declare and initialise
let flag = true;    // boolean variable
x = x + 1;         // reassign (no 'let' on re-assignment)
```

### Arithmetic Operators

| Operator | Meaning | Example |
|----------|---------|---------|
| `+` | Addition | `1 + 2` → `3` |
| `-` | Subtraction | `5 - 3` → `2` |
| `*` | Multiplication | `4 * 3` → `12` |
| `/` | Integer division | `7 / 2` → `3` |
| `%` | Modulo | `7 % 3` → `1` |

Operator precedence matches standard mathematics. Use parentheses to override: `(a + b) * c`.

### Comparison Operators

| Operator | Meaning | Result type |
|----------|---------|-------------|
| `==` | Equal | Boolean |
| `!=` | Not Equal | Boolean |
| `<`  | Less than | Boolean |
| `<=` | Less than or equal | Boolean |
| `>`  | Greater than | Boolean |
| `>=` | Greater than or equal | Boolean |

---

## Architecture Deep-Dive

### Stage 1 — Lexer (`lexer.hpp/cpp`)

The Lexer reads the source string character-by-character and groups characters into **Tokens** — the minimal meaningful units of the language.

Key implementation details:
- Single-pass, `O(n)` scan
- Keyword table backed by `std::unordered_map` for `O(1)` lookup
- Line tracking for error messages
- `//` single-line comment stripping

### Stage 2 — Parser (`parser.hpp/cpp`)

The Parser implements **Recursive Descent Parsing** — one function per grammar rule. It consumes the flat token list and builds a tree (AST) that captures the grammatical structure.

Each level of the grammar handles operators of equal precedence. Lower levels in the call chain bind more tightly — this is how **operator precedence** emerges naturally from the call stack.

### Stage 3 — Compiler (`compiler.hpp/cpp`)

The Compiler walks the AST recursively and **emits bytecode** into a `std::vector<uint8_t>`. Key techniques:

- **Variable slots**: names are mapped to `uint16_t` indices.
- **Jump patching**: for `if`/`while`, placeholders are emitted and then **backpatched** with real offsets.
- **Little-endian encoding**: all multi-byte payloads are written in little-endian byte order.

### Stage 4 — Virtual Machine (`vm.hpp/cpp`)

The VM is a classic **stack machine**:

- **Operand stack**: `std::vector<Value>` — push_back to push, pop_back to pop.
- **Variable array**: `std::vector<Value>` indexed by slot number.
- **Dispatch**: a single `switch` over the opcode byte.

### Instruction Set Architecture (ISA)

| Opcode | Hex | Payload | Stack Effect |
|--------|-----|---------|--------------|
| `PUSH_INT` | `0x01` | 8-byte int64 | `→ int` |
| `PUSH_BOOL` | `0x02` | 1-byte (0/1) | `→ bool` |
| `POP` | `0x03` | — | `v →` |
| `LOAD` | `0x10` | 2-byte slot | `→ var[slot]` |
| `STORE` | `0x11` | 2-byte slot | `v → var[slot]` |
| `ADD` | `0x20` | — | `l r → l+r` |
| `SUB` | `0x21` | — | `l r → l-r` |
| `MUL` | `0x22` | — | `l r → l*r` |
| `DIV` | `0x23` | — | `l r → l/r` |
| `MOD` | `0x24` | — | `l r → l%r` |
| `CMP_EQ` | `0x30` | — | `l r → l==r` |
| `CMP_NE` | `0x31` | — | `l r → l!=r` |
| `CMP_LT` | `0x32` | — | `l r → l<r` |
| `CMP_LE` | `0x33` | — | `l r → l<=r` |
| `CMP_GT` | `0x34` | — | `l r → l>r` |
| `CMP_GE` | `0x35` | — | `l r → l>=r` |
| `JUMP` | `0x40` | 4-byte offset | — |
| `JUMP_IF_FALSE` | `0x41` | 4-byte offset | `v →` |
| `PRINT` | `0x50` | — | `v →` |
| `INPUT` | `0x51` | — | `→ int` |
| `HALT` | `0xFF` | — | — |

---

## References

- **Crafting Interpreters** — Robert Nystrom (nystrom.com/craftinginterpreters)
- **Understanding Stack-Based Virtual Machines** — see JVM spec, CPython dis module
