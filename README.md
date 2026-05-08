# CVM++ — Stack-Based VM & Custom Scripting Language

> *"Most developers use high-level languages without deeply understanding how raw text is translated into instructions a computer can execute. This project demystifies compilers, bytecode, and runtime environments by building them from scratch."*
>
> — CVM++ Project Brief, Coding Club, IIT Guwahati (Even Semester 2026)

---

## Overview

CVM++ is a complete, from-scratch implementation of:

1. A **custom scripting language** (`.cvm` files)
2. A **recursive-descent compiler** that translates source code to proprietary bytecode
3. A **stack-based virtual machine** that executes that bytecode

The entire pipeline — from raw text to execution result — is implemented in ~800 lines of modern C++17 with zero dependencies beyond the standard library.

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
├── src/
│   ├── main.cpp            # CLI entry point (REPL + file runner)
│   ├── lexer.hpp/cpp       # Stage 1: Tokenizer
│   ├── ast.hpp/cpp         # AST node definitions + debug printer
│   ├── parser.hpp/cpp      # Stage 2: Recursive descent parser
│   ├── bytecode.hpp/cpp    # Instruction set architecture (ISA)
│   ├── compiler.hpp/cpp    # Stage 3: AST → Bytecode compiler
│   └── vm.hpp/cpp          # Stage 4: Stack-based execution engine
└── tests/
    ├── hello.cvm           # Hello world / literals
    ├── arithmetic.cvm      # All arithmetic operators + precedence
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

**macOS (Apple Silicon / Intel):** Use Xcode Command Line Tools (`xcode-select --install`) or Homebrew's LLVM. Both arm64 and x86_64 are supported natively.

**Linux:** GCC 9+ or Clang 10+ from your package manager.

**Windows:** Use MinGW-w64, MSYS2, or WSL2.

### Build Steps

```bash
# 1. Clone or enter the project directory
cd cvm++

# 2. Create and enter the build directory
mkdir build && cd build

# 3. Configure (Release mode — optimised binary)
cmake .. -DCMAKE_BUILD_TYPE=Release

# 4. Compile
cmake --build . --parallel

# The binary is at: build/cvm  (or build/cvm.exe on Windows)
```

### Debug Build (with AddressSanitizer + UBSan)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --parallel
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
# Output: 5040
#         true
```

### Debug Mode (`-d`)

Prints the full token list, AST tree, and bytecode disassembly before executing:

```bash
./build/cvm -d tests/factorial.cvm
```

Example debug output snippet:
```
=== Tokens ===
  [LET] "let" (line 6)
  [IDENT] "n" (line 6)
  ...

=== AST ===
[Program]
  [VarDecl: n]
    [Number: 7]
  [While]
    [Condition]
      [BinaryExpr: <]
    ...

=== Bytecode Disassembly ===
Variables:
  [0] n
  [1] result
  [2] i

Instructions:
     0  PUSH_INT  7
     9  STORE  [0] (n)
    12  PUSH_INT  1
    21  STORE  [1] (result)
    ...
    84  JUMP  offset=-53 -> 36
    89  LOAD  [1] (result)
    92  PRINT
    93  HALT
============================

5040
true
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

### Help

```bash
./build/cvm --help
```

### Run Tests

```bash
cd build
ctest --output-on-failure
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

Operator precedence matches standard mathematics: `*` and `/` bind tighter than `+` and `-`. Use parentheses to override: `(a + b) * c`.

### Comparison Operators

| Operator | Meaning | Result type |
|----------|---------|-------------|
| `==` | Equal | Boolean |
| `<`  | Less than | Boolean |

```cvm
let a = 10;
print a == 10;   // true
print a < 5;     // false
```

### Control Flow

#### `if` / `else`

```cvm
if (x < 10) {
    print 1;
} else {
    print 0;
}
```

The `else` clause is optional. Braces `{ }` are always required.

#### `while`

```cvm
let i = 0;
while (i < 5) {
    print i;
    i = i + 1;
}
```

### I/O

```cvm
// Output: print any expression
print x + 1;
print true;

// Input: reads an integer from stdin and binds it to a variable
let n = input;
```

### Comments

```cvm
// This is a single-line comment — everything after // is ignored
let x = 10; // inline comment
```

---

## Architecture Deep-Dive

### Stage 1 — Lexer (`lexer.hpp/cpp`)

The Lexer reads the source string character-by-character and groups characters into **Tokens** — the minimal meaningful units of the language.

```
"let x = 10 + 2;"
  ↓
[LET]["x"][ASSIGN][NUMBER:10][PLUS][NUMBER:2][SEMICOLON]
```

Key implementation details:
- Single-pass, `O(n)` scan
- Keyword table backed by `std::unordered_map` for `O(1)` lookup
- Line tracking for error messages
- `//` single-line comment stripping

### Stage 2 — Parser (`parser.hpp/cpp`)

The Parser implements **Recursive Descent Parsing** — one function per grammar rule. It consumes the flat token list and builds a tree (AST) that captures the grammatical structure.

```
Grammar (simplified):
  expr       := comparison
  comparison := addition ( ('==' | '<') addition )*
  addition   := term     ( ('+' | '-') term     )*
  term       := factor   ( ('*' | '/') factor   )*
  factor     := NUMBER | BOOL | IDENT | '(' expr ')'
```

Each level of the grammar handles operators of equal precedence. Lower levels in the call chain bind more tightly — this is how **operator precedence** emerges naturally from the call stack.

### Stage 3 — Compiler (`compiler.hpp/cpp`)

The Compiler walks the AST recursively and **emits bytecode** into a `std::vector<uint8_t>`. Key techniques:

- **Variable slots**: names are mapped to `uint16_t` indices. The VM stores variable values in a flat array.
- **Jump patching**: for `if`/`while`, a placeholder `0x00000000` offset is emitted, then **backpatched** with the real distance once the target address is known.
- **Little-endian encoding**: all multi-byte payloads (int64, uint16, int32) are written in little-endian byte order.

### Stage 4 — Virtual Machine (`vm.hpp/cpp`)

The VM is a classic **stack machine**:

- **Operand stack**: `std::vector<Value>` — push_back to push, pop_back to pop.
- **Variable array**: `std::vector<Value>` indexed by slot number.
- **Instruction pointer (IP)**: `size_t` index into `Chunk::code`.
- **Dispatch**: a single `switch` over the opcode byte. On ARM64 (Apple Silicon), Clang/GCC generate efficient jump tables from this pattern.

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
| `CMP_EQ` | `0x30` | — | `l r → l==r` |
| `CMP_LT` | `0x31` | — | `l r → l<r` |
| `JUMP` | `0x40` | 4-byte offset | — |
| `JUMP_IF_FALSE` | `0x41` | 4-byte offset | `v →` |
| `PRINT` | `0x50` | — | `v →` |
| `INPUT` | `0x51` | — | `→ int` |
| `HALT` | `0xFF` | — | — |

Jump offsets are **relative to the end of the jump instruction** (i.e., relative to the position after the 4-byte payload). A negative offset jumps backward (used for `while` loops).

---

## Sample Script Walkthrough

Here is how `factorial.cvm` translates through the pipeline:

**Source:**
```cvm
let n = 7;
let result = 1;
let i = 1;
while (i < n + 1) {
    result = result * i;
    i = i + 1;
}
print result;
```

**Bytecode (from `-d` mode):**
```
 0  PUSH_INT  7        ; push literal 7
 9  STORE  [0] (n)     ; n = 7
12  PUSH_INT  1        ; push literal 1
21  STORE  [1] (result); result = 1
24  PUSH_INT  1
33  STORE  [2] (i)     ; i = 1
──── loop start ────
36  LOAD  [2] (i)      ; push i
39  LOAD  [0] (n)      ; push n
42  PUSH_INT  1
51  ADD                ; n + 1
52  CMP_LT             ; i < (n+1)  → bool
53  JUMP_IF_FALSE  → 89; exit if false
58  LOAD  [1] (result)
61  LOAD  [2] (i)
64  MUL                ; result * i
65  STORE  [1] (result); result = result * i
68  LOAD  [2] (i)
71  PUSH_INT  1
80  ADD                ; i + 1
81  STORE  [2] (i)     ; i = i + 1
84  JUMP  → 36         ; back to loop top
──── loop end ────
89  LOAD  [1] (result)
92  PRINT              ; print 5040
93  HALT
```

---

## Possible Extensions

Once you've understood the core pipeline, here are natural next steps:

| Feature | Hint |
|---------|------|
| `>` and `>=` operators | Add tokens + parser rule + `CMP_GT`/`CMP_GTE` opcodes |
| Negation (`-x`) | `UnaryMinus` AST node + `NEG` opcode |
| String literals | Add `STRING` token type + `Value` string variant |
| Functions | Symbol table scoping + `CALL`/`RETURN` opcodes + a call stack |
| Arrays | `ALLOC` / `INDEX` opcodes + heap-style Value storage |
| Error recovery | Parser: `synchronize()` after a parse error, continue parsing |
| Bytecode serialization | Write/read `Chunk` to `.cvmb` binary files |

---

## References

- **Crafting Interpreters** — Robert Nystrom (nystrom.com/craftinginterpreters) — The definitive guide this project is architecturally inspired by.
- **Writing a Lexer in C++** — any compiler course material
- **Understanding Stack-Based Virtual Machines** — see JVM spec, CPython dis module

---

*CVM++ — Coding Club, IIT Guwahati | Even Semester Projects 2026*
