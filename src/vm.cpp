// =============================================================================
// vm.cpp — CVM++ Virtual Machine Implementation
//
// The heart of CVM++: a dispatch loop that reads one opcode at a time,
// decodes its payload (if any), and applies the operation to the stack.
// =============================================================================

#include "vm.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>

// =============================================================================
// Value Helpers
// =============================================================================

std::string valueToString(const Value& v) {
    if (std::holds_alternative<int64_t>(v))
        return std::to_string(std::get<int64_t>(v));
    return std::get<bool>(v) ? "true" : "false";
}

int64_t asInt(const Value& v, const char* context) {
    if (!std::holds_alternative<int64_t>(v))
        throw RuntimeError(std::string("TypeError: expected integer in ") + context);
    return std::get<int64_t>(v);
}

bool asBool(const Value& v, const char* context) {
    if (!std::holds_alternative<bool>(v))
        throw RuntimeError(std::string("TypeError: expected bool in ") + context);
    return std::get<bool>(v);
}

bool isTruthy(const Value& v) {
    if (std::holds_alternative<int64_t>(v)) return std::get<int64_t>(v) != 0;
    return std::get<bool>(v);
}

// =============================================================================
// VM — Main Execution Loop
// =============================================================================

void VM::execute(const Chunk& chunk) {
    // Reset state for a fresh run
    m_stack.clear();
    m_vars.clear();
    m_vars.resize(chunk.varNames.size()); // pre-allocate all variable slots
    m_ip = 0;

    const auto& code = chunk.code;

    // -------------------------------------------------------------------------
    // The dispatch loop: fetch → decode → execute, repeat until HALT.
    // Using a raw switch over uint8_t gives the compiler the best chance to
    // generate a jump table on ARM64 (Apple Silicon).
    // -------------------------------------------------------------------------
    while (m_ip < code.size()) {
        uint8_t raw = code[m_ip++];
        Opcode  op  = static_cast<Opcode>(raw);

        switch (op) {

            // -----------------------------------------------------------------
            // PUSH_INT  [8 bytes: little-endian int64_t]
            // Push an immediate integer constant onto the stack.
            // -----------------------------------------------------------------
            case Opcode::PUSH_INT: {
                int64_t v = readInt64(code);
                push(v);
                break;
            }

            // -----------------------------------------------------------------
            // PUSH_BOOL  [1 byte: 0 or 1]
            // Push an immediate boolean constant.
            // -----------------------------------------------------------------
            case Opcode::PUSH_BOOL: {
                bool v = (code[m_ip++] != 0);
                push(v);
                break;
            }

            // -----------------------------------------------------------------
            // POP — discard top of stack
            // -----------------------------------------------------------------
            case Opcode::POP: {
                pop();
                break;
            }

            // -----------------------------------------------------------------
            // LOAD  [2 bytes: uint16_t slot index]
            // Push the value of a variable onto the stack.
            // -----------------------------------------------------------------
            case Opcode::LOAD: {
                uint16_t slot = readUint16(code);
                push(getVar(slot));
                break;
            }

            // -----------------------------------------------------------------
            // STORE  [2 bytes: uint16_t slot index]
            // Pop the top of stack and write it into the variable slot.
            // -----------------------------------------------------------------
            case Opcode::STORE: {
                uint16_t slot = readUint16(code);
                setVar(slot, pop());
                break;
            }

            // -----------------------------------------------------------------
            // Arithmetic: pop right, pop left, push result.
            // All arithmetic requires integer operands.
            // -----------------------------------------------------------------
            case Opcode::ADD: {
                Value r = pop(); Value l = pop();
                push(asInt(l, "ADD") + asInt(r, "ADD"));
                break;
            }
            case Opcode::SUB: {
                Value r = pop(); Value l = pop();
                push(asInt(l, "SUB") - asInt(r, "SUB"));
                break;
            }
            case Opcode::MUL: {
                Value r = pop(); Value l = pop();
                push(asInt(l, "MUL") * asInt(r, "MUL"));
                break;
            }
            case Opcode::DIV: {
                Value r = pop(); Value l = pop();
                int64_t divisor = asInt(r, "DIV");
                if (divisor == 0)
                    throw RuntimeError("RuntimeError: division by zero");
                push(asInt(l, "DIV") / divisor);
                break;
            }

            // -----------------------------------------------------------------
            // CMP_EQ — equality comparison (works for both int and bool)
            // Pushes a bool result.
            // -----------------------------------------------------------------
            case Opcode::CMP_EQ: {
                Value r = pop(); Value l = pop();
                push(l == r); // std::variant operator== compares type + value
                break;
            }

            // -----------------------------------------------------------------
            // CMP_LT — less-than comparison (integers only)
            // Pushes a bool result.
            // -----------------------------------------------------------------
            case Opcode::CMP_LT: {
                Value r = pop(); Value l = pop();
                push(asInt(l, "CMP_LT") < asInt(r, "CMP_LT"));
                break;
            }

            // -----------------------------------------------------------------
            // JUMP  [4 bytes: little-endian int32_t relative offset]
            // Unconditionally move the IP by offset from post-payload position.
            // -----------------------------------------------------------------
            case Opcode::JUMP: {
                int32_t offset = readInt32(code);
                // m_ip already points past the 4-byte payload
                m_ip = static_cast<std::size_t>(
                    static_cast<int64_t>(m_ip) + offset);
                break;
            }

            // -----------------------------------------------------------------
            // JUMP_IF_FALSE  [4 bytes: int32_t relative offset]
            // Pop condition; jump only if it is falsy.
            // -----------------------------------------------------------------
            case Opcode::JUMP_IF_FALSE: {
                int32_t offset = readInt32(code);
                Value cond = pop();
                if (!isTruthy(cond)) {
                    m_ip = static_cast<std::size_t>(
                        static_cast<int64_t>(m_ip) + offset);
                }
                break;
            }

            // -----------------------------------------------------------------
            // PRINT — pop and display the top of stack
            // -----------------------------------------------------------------
            case Opcode::PRINT: {
                Value v = pop();
                std::cout << valueToString(v) << "\n";
                break;
            }

            // -----------------------------------------------------------------
            // INPUT — read an integer from stdin and push it
            // -----------------------------------------------------------------
            case Opcode::INPUT: {
                int64_t v;
                std::cout << "> "; // prompt
                if (!(std::cin >> v))
                    throw RuntimeError("RuntimeError: failed to read integer from input");
                push(v);
                break;
            }

            // -----------------------------------------------------------------
            // HALT — end of program
            // -----------------------------------------------------------------
            case Opcode::HALT:
                return;

            default:
                throw RuntimeError("RuntimeError: unknown opcode 0x" +
                    std::to_string(raw) + " at ip=" + std::to_string(m_ip - 1));
        }
    }
}

// =============================================================================
// Stack Operations
// =============================================================================

void VM::push(Value v) {
    m_stack.push_back(std::move(v));
}

Value VM::pop() {
    if (m_stack.empty())
        throw RuntimeError("RuntimeError: stack underflow");
    Value v = std::move(m_stack.back());
    m_stack.pop_back();
    return v;
}

Value& VM::top() {
    if (m_stack.empty())
        throw RuntimeError("RuntimeError: stack underflow on peek");
    return m_stack.back();
}

// =============================================================================
// Variable Access
// =============================================================================

Value VM::getVar(uint16_t slot) const {
    if (slot >= m_vars.size())
        throw RuntimeError("RuntimeError: invalid variable slot " + std::to_string(slot));
    return m_vars[slot];
}

void VM::setVar(uint16_t slot, Value v) {
    if (slot >= m_vars.size())
        throw RuntimeError("RuntimeError: invalid variable slot " + std::to_string(slot));
    m_vars[slot] = std::move(v);
}

// =============================================================================
// Payload Readers — consume bytes from code[m_ip] and advance m_ip
// All formats are little-endian (matches the compiler's emitters).
// =============================================================================

int64_t VM::readInt64(const std::vector<uint8_t>& code) {
    int64_t v;
    std::memcpy(&v, code.data() + m_ip, 8);
    m_ip += 8;
    return v;
}

uint16_t VM::readUint16(const std::vector<uint8_t>& code) {
    uint16_t v;
    std::memcpy(&v, code.data() + m_ip, 2);
    m_ip += 2;
    return v;
}

int32_t VM::readInt32(const std::vector<uint8_t>& code) {
    int32_t v;
    std::memcpy(&v, code.data() + m_ip, 4);
    m_ip += 4;
    return v;
}
