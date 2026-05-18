#pragma once
// =============================================================================
// bytecode.hpp — CVM++ Instruction Set Architecture (ISA)
//
// Defines the complete set of opcodes (operation codes) for the CVM++
// bytecode format. Each opcode is a single uint8_t value.
//
// Bytecode layout:
//   The compiled output is a flat std::vector<uint8_t>. Instructions are
//   variable-width:
//     * Most opcodes are 1 byte.
//     * PUSH_INT    is followed by 8 bytes (little-endian int64_t).
//     * PUSH_BOOL   is followed by 1 byte  (0 = false, 1 = true).
//     * LOAD / STORE is followed by 2 bytes (uint16_t variable index).
//     * JUMP / JUMP_IF_FALSE is followed by 4 bytes (int32_t offset from
//       current instruction pointer, pointing AFTER the jump instruction).
//
// Stack machine semantics:
//   - All arithmetic and logic operates on a value stack.
//   - LOAD pushes a variable's value onto the stack.
//   - STORE pops the top of stack into a variable slot.
//   - BINARY_* pops two operands and pushes one result.
// =============================================================================

#include <cstdint>
#include <string>

// -----------------------------------------------------------------------------
// Opcode — single-byte instruction codes
// -----------------------------------------------------------------------------
enum class Opcode : uint8_t {
    // --- Stack Manipulation ---
    PUSH_INT        = 0x01, // push immediate int64  [8 bytes payload]
    PUSH_BOOL       = 0x02, // push immediate bool   [1 byte payload: 0/1]
    POP             = 0x03, // discard top of stack

    // --- Variables ---
    LOAD            = 0x10, // push variable[idx] onto stack [2 bytes: uint16_t]
    STORE           = 0x11, // pop stack → variable[idx]     [2 bytes: uint16_t]

    // --- Arithmetic (pop 2, push 1) ---
    ADD             = 0x20,
    SUB             = 0x21,
    MUL             = 0x22,
    DIV             = 0x23,

    // --- Comparison (pop 2, push bool) ---
    CMP_EQ          = 0x30, // ==
    CMP_LT          = 0x31, // <

    // --- Control Flow ---
    JUMP            = 0x40, // unconditional jump  [4 bytes: int32_t target offset]
    JUMP_IF_FALSE   = 0x41, // pop; jump if false  [4 bytes: int32_t target offset]

    // --- I/O ---
    PRINT           = 0x50, // pop and print top of stack
    INPUT           = 0x51, // read integer from stdin; push onto stack

    // --- Program Lifecycle ---
    HALT            = 0xFF, // terminate execution
};

// Human-readable opcode name (for disassembly output)
const char* opcodeName(Opcode op);
