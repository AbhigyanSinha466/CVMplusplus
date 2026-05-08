#pragma once
// =============================================================================
// vm.hpp — CVM++ Stack-Based Virtual Machine
//
// The VM is the FOURTH and final stage of the pipeline:
//   Chunk (bytecode)  -->  [VM]  -->  Output / Side effects
//
// Architecture overview:
//   - A flat execution loop: one big switch over the current opcode.
//   - Stack: std::vector<Value> used as a LIFO operand stack.
//     (top of stack = back of vector; push = push_back, pop = pop_back)
//   - Variables: std::vector<Value> indexed by uint16_t slot.
//   - IP (instruction pointer): std::size_t index into chunk.code.
//
// Value type:
//   Values can be integers OR booleans. We use a tagged union (Value)
//   to represent both without heap allocation.
// =============================================================================

#include "compiler.hpp"  // Chunk
#include <vector>
#include <variant>
#include <string>
#include <stdexcept>
#include <cstdint>

// -----------------------------------------------------------------------------
// Value — A discriminated union holding either an int64_t or bool.
// We use std::variant (C++17) for type safety and zero-cost construction.
// -----------------------------------------------------------------------------
using Value = std::variant<int64_t, bool>;

// Helper: convert a Value to a printable string
std::string valueToString(const Value& v);

// Helper: extract int64_t from Value or throw a runtime type error
int64_t asInt(const Value& v, const char* context);

// Helper: extract bool from Value or throw a runtime type error
bool asBool(const Value& v, const char* context);

// Helper: coerce any Value to bool (0 / false = falsy)
bool isTruthy(const Value& v);

// -----------------------------------------------------------------------------
// RuntimeError — Thrown when the VM encounters a fatal error during execution.
// -----------------------------------------------------------------------------
struct RuntimeError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// -----------------------------------------------------------------------------
// VM — Executes a compiled Chunk.
// -----------------------------------------------------------------------------
class VM {
public:
    // Execute the bytecode chunk. Returns normally on HALT.
    // Throws RuntimeError on any execution failure.
    void execute(const Chunk& chunk);

private:
    // --- Internal state (reset on each execute() call) ---
    std::vector<Value>  m_stack;     // operand stack
    std::vector<Value>  m_vars;      // variable storage (indexed by slot)
    std::size_t         m_ip;        // instruction pointer

    // --- Stack operations ---
    void  push(Value v);
    Value pop();
    Value& top();   // peek without consuming

    // --- Variable access ---
    Value getVar(uint16_t slot) const;
    void  setVar(uint16_t slot, Value v);

    // --- Little-endian payload readers ---
    int64_t  readInt64(const std::vector<uint8_t>& code);
    uint16_t readUint16(const std::vector<uint8_t>& code);
    int32_t  readInt32(const std::vector<uint8_t>& code);
};
